
// With gcc, remove all deprecated warning from ffmpeg
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <functional>
#include <iostream>

#include "h264.h"
#include "tools.h"
#include "Log.h"
#include "ReadFileChunk.h"

#ifndef INT64_C
#define INT64_C(c) (c##LL)
#define UINT64_C(c) (c##ULL)
#endif

#ifdef _MSC_VER
#define NUM_THREADS_INTERNAL 1 // 4
#define NUM_THREADS_H264 1
#else
#define NUM_THREADS_INTERNAL 1 // 4
#define NUM_THREADS_H264 1
#endif

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavcodec/avfft.h>

#include <libavdevice/avdevice.h>

#include <libavfilter/avfilter.h>
	// #include <libavfilter/avfiltergraph.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>

#include <libavformat/avformat.h>
#include <libavformat/avio.h>

	// libav resample

#include <libavutil/opt.h>
#include <libavutil/common.h>
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/file.h>
#include <libswscale/swscale.h>

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
}

#include <fstream>
#include <set>
#include <cmath>
#include <stdexcept>
#include <thread>
#include <map>
#include <mutex>

#include "BadPixels.h"

static int decode(AVCodecContext *dec_ctx, AVFrame *frame, int *got_frame, AVPacket *pkt)
{
	int used = 0;
	if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO ||
		dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO)
	{
		used = avcodec_send_packet(dec_ctx, pkt);
		if (used < 0 && used != AVERROR(EAGAIN) && used != AVERROR_EOF)
		{
		}
		else
		{
			// if (used >= 0)
			//	pkt->size = 0;
			used = avcodec_receive_frame(dec_ctx, frame);
			if (used >= 0)
				*got_frame = 1;
			//             if (used == AVERROR(EAGAIN) || used == AVERROR_EOF)
			//                 used = 0;
		}
	}
	return used;
}
namespace rir
{

	static double std_dev_diff(const unsigned short *p1, const unsigned short *p2, int size)
	{
		double x = 0.0;
		double x_squared = 0.0;

		for (int i = 0; i < size; ++i)
		{
			int diff = (int)p1[i] - (int)p2[i];
			diff = (diff < 0 ? -diff : diff);
			x += diff;
			x_squared += diff * diff;
		}

		return std::sqrt((x_squared - (x * x) / size) /
						 (size - 1));
	}

	static double mean_image(const unsigned short *p, int size)
	{
		std::int64_t sum1 = 0, sum2 = 0;

		int size2 = size / 2;
		for (int i = 0; i < size2; ++i)
			sum1 += p[i];
		for (int i = size2; i < size; ++i)
			sum2 += p[i];

		sum1 += sum2;
		return (double)sum1 / size;
	}

	static std::pair<double, double> mean_std(const double *p, int size)
	{
		double x = 0.0;
		double x_squared = 0.0;

		for (int i = 0; i < size; ++i)
		{
			x += p[i];
			x_squared += p[i] * p[i];
		}

		return {x / size, std::sqrt((x_squared - (x * x) / size) /
									(size - 1))};
	}

	static void max_image(unsigned short *dst, const unsigned short *im, int width, int height, int lossy_height)
	{
		int lossy_size = width * lossy_height;
		for (int i = 0; i < lossy_size; ++i)
			dst[i] = std::max(dst[i], im[i]);

		// forward last lines
		if (height != lossy_height)
		{
			memcpy(dst + lossy_size, im + lossy_size, (height - lossy_height) * width * 2);
		}
	}

	class VideoDownsampler::PrivateData
	{
	public:
		int width;
		int height;
		int lossy_height;
		int factor;
		double factor_std;
		callback_type callback;

		std::vector<double> std_dev;
		std::vector<unsigned short> prev;
		std::vector<unsigned short> last_saved;
		int buffer_size;

		int part;
		std::vector<double> std_dev_copy;
		int last_added;
		int count;
		std::vector<unsigned short> max_im;
		std::int64_t max_im_time;
		std::vector<std::int64_t> timestamps;
		int i;
		std::map<std::string, std::string> attributes;

		PrivateData(int _width, int _height, int _lossy_height, int _factor, double _factor_std, callback_type _callback)
			: width(_width), height(_height), lossy_height(_lossy_height), factor(_factor), factor_std(_factor_std), callback(_callback)
		{
			prev.resize(width * height);
			last_saved.resize(width * height);
			buffer_size = 96;

			// factor_std = (1. - (1. / factor));
			// double factor2 = (factor * 1.);
			// part = (int)((buffer_size / factor2) * (factor2 - 1));
			part = (int)(factor_std * buffer_size);
			if (part < 0)
				part = 0;
			else if (part >= buffer_size)
				part = buffer_size - 1;
			std_dev_copy.resize(buffer_size);
			last_added = 0;
			count = 0;
			max_im.resize(width * height);
			std::fill_n(max_im.data(), max_im.size(), 0);
			max_im_time = 0;
			i = 0;
		}
	};

	VideoDownsampler::VideoDownsampler()
		: d_data(nullptr)
	{
	}
	VideoDownsampler::~VideoDownsampler()
	{
		if (d_data)
			delete d_data;
	}

	bool VideoDownsampler::open(int width, int height, int lossy_height, int factor, double factor_std, callback_type callback)
	{
		if (width <= 0 || height <= 0 || lossy_height > height || factor < 1 || factor_std < 0 || factor_std > 1 || !callback)
			return false;

		if (d_data)
		{
			delete d_data;
			d_data = nullptr;
		}
		d_data = new PrivateData(width, height, lossy_height, factor, factor_std, callback);
		return true;
	}

	int VideoDownsampler::close()
	{
		if (!d_data)
			return 0;
		int res = d_data->count;
		delete d_data;
		d_data = nullptr;
		return res;
	}

	bool VideoDownsampler::addImage2(const unsigned short *img, std::int64_t timestamp, const std::map<std::string, std::string> &attributes)
	{
		if (!d_data)
			return false;

		if (!d_data->timestamps.empty() && timestamp <= d_data->timestamps.back())
			return false;

		d_data->timestamps.push_back(timestamp);

		if (d_data->factor == 1)
		{
			d_data->callback(img, timestamp, attributes);
			d_data->i++;
			d_data->count++;
			return true;
		}

		if (d_data->i == 0)
		{
			// update prev image
			memcpy(d_data->prev.data(), img, d_data->width * d_data->height * 2);
			d_data->i++;
			d_data->callback(img, timestamp, attributes);
			d_data->last_added = 0;
			d_data->count++;
			return true;
		}

		double current_std = std_dev_diff(img, d_data->prev.data(), d_data->width * d_data->lossy_height);
		if (d_data->std_dev.size() < 10)
		{
			d_data->std_dev.push_back(current_std);
			// compute maximum image
			max_image(d_data->max_im.data(), img, d_data->width, d_data->height, d_data->lossy_height);
			d_data->max_im_time = timestamp;
			d_data->attributes = attributes;

			if (d_data->i % d_data->factor == 0)
			{
				// add image
				d_data->callback(d_data->max_im.data(), d_data->max_im_time, d_data->attributes);
				d_data->last_added = d_data->i;
				d_data->count++;
				// memcpy(d_data->last_saved.data(), img, d_data->width * d_data->height*2);
				//  reset max image
				std::fill_n(d_data->max_im.data(), d_data->max_im.size(), 0);
			}
			// update prev image
			memcpy(d_data->prev.data(), img, d_data->width * d_data->height * 2);
			d_data->i++;
			return true;
		}

		auto tmp = mean_std(d_data->std_dev.data(), d_data->std_dev.size());
		double mean = tmp.first;
		double std = tmp.second;

		// compute maximum image
		max_image(d_data->max_im.data(), img, d_data->width, d_data->height, d_data->lossy_height);
		d_data->max_im_time = timestamp;
		d_data->attributes = attributes;

		double std_ratio = std / mean;
		bool nothing = std_ratio < 0.1;
		if (nothing)
			nothing = nothing && (current_std < mean + 2 * std);
		// if (nothing == true && current_std > mean + 0.5 * std)
		//	printf("nothing %i\n", d_data->i);

		if (d_data->i % d_data->factor == 0 || (!nothing && current_std > mean + 0.5 * std))
		{
			// add image
			d_data->callback(d_data->max_im.data(), d_data->max_im_time, attributes);
			d_data->last_added = d_data->i;
			d_data->count++;

			// reset max image
			std::fill_n(d_data->max_im.data(), d_data->max_im.size(), 0);
		}
		if ((current_std < mean + 5 * std && current_std > mean - std) || d_data->i % d_data->factor == 0)
		{
			if (d_data->std_dev.size() < d_data->buffer_size)
				d_data->std_dev.push_back(current_std);
			else
			{
				// update array of standard deviation ONLY if current image is not completely different from previous one
				memmove(d_data->std_dev.data(), d_data->std_dev.data() + 1, (d_data->std_dev.size() - 1) * sizeof(double));
				d_data->std_dev.back() = current_std;
			}
		}

		// update prev image
		memcpy(d_data->prev.data(), img, d_data->width * d_data->height * 2);
		d_data->i++;
		return true;
	}

	bool VideoDownsampler::addImage(const unsigned short *img, std::int64_t timestamp, const std::map<std::string, std::string> &attributes)
	{
		if (!d_data)
			return false;

		if (!d_data->timestamps.empty() && timestamp <= d_data->timestamps.back())
			return false;

		d_data->timestamps.push_back(timestamp);

		if (d_data->factor == 1)
		{
			d_data->callback(img, timestamp, attributes);
			d_data->i++;
			d_data->count++;
			return true;
		}

		if (d_data->std_dev.size() < d_data->buffer_size)
		{

			if (d_data->i > 0)
			{
				// Add standardd eviation of difference of 2 consecutive images
				double std = std_dev_diff(img, d_data->prev.data(), d_data->width * d_data->lossy_height);
				d_data->std_dev.push_back(std);
			}

			// compute maximum image
			max_image(d_data->max_im.data(), img, d_data->width, d_data->height, d_data->lossy_height);
			d_data->max_im_time = timestamp;
			d_data->attributes = attributes;

			if (d_data->i % d_data->factor == 0)
			{
				// add image
				d_data->callback(d_data->max_im.data(), d_data->max_im_time, d_data->attributes);
				d_data->last_added = d_data->i;
				d_data->count++;
				// memcpy(d_data->last_saved.data(), img, d_data->width * d_data->height*2);
				//  reset max image
				std::fill_n(d_data->max_im.data(), d_data->max_im.size(), 0);
			}
		}
		else
		{

			std::copy(d_data->std_dev.begin(), d_data->std_dev.end(), d_data->std_dev_copy.begin());
			std::nth_element(d_data->std_dev_copy.begin(), d_data->std_dev_copy.begin() + d_data->part, d_data->std_dev_copy.end());
			double val = d_data->std_dev_copy[d_data->part];

			auto tmp = mean_std(d_data->std_dev.data(), d_data->std_dev.size());
			double mean = tmp.first;
			double std = tmp.second;

			double current_std = std_dev_diff(img, d_data->prev.data(), d_data->width * d_data->lossy_height);

			double nothing_factor = 0.5;
			bool nothing = current_std < tmp.first - nothing_factor * tmp.second;
			if ((d_data->i - d_data->last_added) >= (d_data->factor * 2))
				nothing = false;

			// compute maximum image
			max_image(d_data->max_im.data(), img, d_data->width, d_data->height, d_data->lossy_height);
			d_data->max_im_time = timestamp;

			if ((current_std > val || (d_data->i - d_data->last_added) >= (d_data->factor)) && !nothing)
			{
				// Something new that must be recorded

				// d_data->attributes = attributes;

				// add image
				d_data->callback(d_data->max_im.data(), d_data->max_im_time, attributes);
				d_data->last_added = d_data->i;
				d_data->count++;

				// reset max image
				std::fill_n(d_data->max_im.data(), d_data->max_im.size(), 0);
			}

			if (current_std < mean + 10 * std)
			{
				// update array of standard deviation ONLY if current image is not completely different from previous one
				memmove(d_data->std_dev.data(), d_data->std_dev.data() + 1, (d_data->std_dev.size() - 1) * sizeof(double));
				d_data->std_dev.back() = current_std;
			}
		}

		// update prev image
		memcpy(d_data->prev.data(), img, d_data->width * d_data->height * 2);
		d_data->i++;
		return true;
	}

	int init_libavcodec()
	{
		// av_register_all();
		// avcodec_register_all();
		avdevice_register_all();
		// avfilter_register_all();
		// avcodec_init ();
		return 0;
	}
	static int init = init_libavcodec();

	static const char *compressionToPreset(int level)
	{
		static const char *ultrafast = "ultrafast";
		static const char *superfast = "superfast";
		static const char *veryfast = "veryfast";
		static const char *faster = "faster";
		static const char *fast = "fast";
		static const char *medium = "medium";
		static const char *slow = "slow";
		static const char *slower = "slower";
		static const char *veryslow = "veryslow";

		if (level <= 0)
			return ultrafast;
		else if (level == 1)
			return superfast;
		else if (level == 2)
			return veryfast;
		else if (level == 3)
			return faster;
		else if (level == 4)
			return fast;
		else if (level == 5)
			return medium;
		else if (level == 6)
			return slow;
		else if (level == 7)
			return slower;
		else
			return veryslow;
	}

	static int presetToLevel(const char *preset)
	{
		if (strcmp(preset, "ultrafast") == 0)
			return 0;
		if (strcmp(preset, "superfast") == 0)
			return 1;
		if (strcmp(preset, "veryfast") == 0)
			return 2;
		if (strcmp(preset, "faster") == 0)
			return 3;
		if (strcmp(preset, "fast") == 0)
			return 4;
		if (strcmp(preset, "medium") == 0)
			return 5;
		if (strcmp(preset, "slow") == 0)
			return 6;
		if (strcmp(preset, "slower") == 0)
			return 7;
		return 0;
	}

	class H264Capture
	{
	public:
		H264Capture()
		{
			oformat = NULL;
			ofctx = NULL;
			videoStream = NULL;
			videoFrame = NULL;
			swsCtx = NULL;
			frameCounter = 0;
			lastKeyFrame = 0;
			GOP = 30;
		}
		~H264Capture()
		{
			Free();
		}
		bool Init(const char *filename, int width, int height, int fpsrate, const char *preset, const char *codecn = "h264", int GOP = 50, int _threads = NUM_THREADS_H264, int slices = 1);
		bool AddFrame(const unsigned short *img, bool key = false);
		bool AddFrame(const unsigned short *img, const unsigned char *IT, bool key = false);
		bool Finish();
		bool Initialized() const
		{
			return oformat != NULL;
		}
		int Count() const
		{
			return frameCounter;
		}

	private:
		std::string codec_name;
		std::string fname;
		std::string tmp_name;
		std::string threadCount;
		const AVOutputFormat *oformat;
		AVFormatContext *ofctx;
		AVStream *videoStream;
		AVFrame *videoFrame;
		const AVCodec *codec;
		const AVCodec *kvazaar;
		AVCodecContext *cctx;
		SwsContext *swsCtx;
		std::vector<uint8_t> img;
		int frameCounter;
		AVPixelFormat file_format;
		int fps;
		int crf;
		int lastKeyFrame;
		int GOP;
		int frame_width;
		int frame_height;
		void Free();
		bool Remux();
	};

	bool H264Capture::Init(const char *filename, int width, int height, int fpsrate, const char *preset, const char *codecn, int _GOP, int _threads, int slices)
	{

		fname = filename;
		fps = fpsrate;
		codec_name = "h264_nvenc";//codecn;// "libx264"; // codecn;
		std::string ext = codec_name;
		frame_width = width;
		frame_height = height;

		// For now, we only authorize h264 and h265 codecs
		/*if (ext != "h264" && ext != "h265") {
			RIR_LOG_ERROR("Wrong codec name: %s", codecn);
			return false;
		}*/

		if (ext == "vp9")
			ext = "webm";
		else if (ext == "vp8")
			ext = "ogv";
		else if (ext == "dirac")
			ext = "vc2";
		else if (ext == "ffv1")
			ext = "mkv";
		else if (ext == "av1")
			ext = "avi";

		tmp_name = fname + "." + ext;
		threadCount = toString(_threads);

		const AVCodec *libx264 = nullptr;
		if (codec_name == "h264_nvenc")
		{
			libx264 = avcodec_find_encoder_by_name("h264_nvenc");
			codec_name = "h264";
			tmp_name = fname + ".h264";
			ext = "h264";
		}

		int err;
		if (!(oformat = av_guess_format(NULL, tmp_name.c_str(), NULL)))
		{
			if (codec_name == "h264")
			{
				// This might be the LGPL version of ffmpeg, retry with h265 codec and libkvazaar
				codec_name = "h265";
				ext = codec_name;
				tmp_name = fname + "." + ext;
				if (!(oformat = av_guess_format(NULL, tmp_name.c_str(), NULL)))
				{
					RIR_LOG_ERROR("Failed to define output format for file %s", tmp_name.c_str());
					return false;
				}
			}
			else
			{
				RIR_LOG_ERROR("Failed to define output format %s", tmp_name.c_str());
				return false;
			}
		}

		// detect kvazaar video codec
		kvazaar = NULL;
		if (codec_name == "h265")
		{
			kvazaar = av_codec_iterate(NULL);
			while (kvazaar)
			{
				if (kvazaar->id == AV_CODEC_ID_HEVC && strcmp(kvazaar->name, "libkvazaar") == 0)
					break;
				kvazaar = av_codec_iterate((void **)kvazaar);
			}
		}

		if ((err = avformat_alloc_output_context2(&ofctx, oformat, NULL, tmp_name.c_str()) < 0))
		{
			RIR_LOG_ERROR("Failed to allocate output context (%i)", err);
			Free();
			return false;
		}

		AVCodecID id = codec_name == "h265" ? AV_CODEC_ID_HEVC : oformat->video_codec;
		if (codec_name == "ffv1")
			id = AV_CODEC_ID_FFV1;
		if (codec_name == "av1")
			id = AV_CODEC_ID_AV1;

		if (kvazaar)
			codec = kvazaar;
		if (libx264)
			codec = libx264;
		else if (!(codec = avcodec_find_encoder(/*oformat->video_codec*/ id)))
		{
			RIR_LOG_ERROR("Failed to find encoder for id %i", (int)id);
			Free();
			return false;
		}

		if (!(videoStream = avformat_new_stream(ofctx, codec)))
		{
			RIR_LOG_ERROR("Failed to create new stream ");
			Free();
			return false;
		}

		if (!(cctx = avcodec_alloc_context3(codec)))
		{
			RIR_LOG_ERROR("Failed to allocate codec context");
			Free();
			return false;
		}

		videoStream->time_base = {1, fps};

		this->file_format = AV_PIX_FMT_YUV444P;
		videoStream->codecpar->codec_id = /*oformat->video_codec*/ id;
		videoStream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
		videoStream->codecpar->width = width;
		videoStream->codecpar->height = height;
		videoStream->codecpar->format = file_format;

		cctx->time_base = {1, fps};
		AVDictionary *av_dict_opts = nullptr;

		// av_log_set_level(AV_LOG_TRACE);

		if (videoStream->codecpar->codec_id == AV_CODEC_ID_AV1)
		{
			cctx->gop_size = _GOP;
			cctx->time_base = {1, fps};
			cctx->pix_fmt = file_format;
			cctx->width = width;
			cctx->height = height;
			av_opt_set(cctx->priv_data, "threads", threadCount.c_str(), AV_OPT_SEARCH_CHILDREN);
			av_opt_set(cctx, "threads", threadCount.c_str(), AV_OPT_SEARCH_CHILDREN);
			av_dict_set(&av_dict_opts, "threads", threadCount.c_str(), AV_DICT_MATCH_CASE);

			int level = presetToLevel(preset);
			level = 8 - level;
			std::string _level = toString(level);
			av_opt_set(cctx->priv_data, "aom-params", ("lossless=1:row-mt=1:cpu-used=" + _level).c_str(), AV_OPT_SEARCH_CHILDREN);
			av_opt_set(cctx, "aom-params", ("lossless=1:row-mt=1:cpu-used=" + _level).c_str(), AV_OPT_SEARCH_CHILDREN);
			av_dict_set(&av_dict_opts, "aom-params", ("lossless=1:row-mt=1:cpu-used=" + _level).c_str(), AV_DICT_MATCH_CASE);

			av_opt_set(cctx->priv_data, "crf", "0", AV_OPT_SEARCH_CHILDREN);
			av_opt_set(cctx, "crf", "0", AV_OPT_SEARCH_CHILDREN);
			av_dict_set(&av_dict_opts, "crf", "0", AV_DICT_MATCH_CASE);

			av_opt_set(cctx->priv_data, "tiles", "2x2", AV_OPT_SEARCH_CHILDREN);
			av_opt_set(cctx, "tiles", "2x2", AV_OPT_SEARCH_CHILDREN);
			av_dict_set(&av_dict_opts, "tiles", "2x2", AV_DICT_MATCH_CASE);

			/*cctx->bit_rate=0;
			cctx->qmin = cctx->qmax = 0;
			cctx->thread_count = _threads;*/

			/*av_opt_set(cctx->priv_data, "b", "0", AV_OPT_SEARCH_CHILDREN);
			av_opt_set(cctx, "b", "0", AV_OPT_SEARCH_CHILDREN);
			av_dict_set(&av_dict_opts, "b", "0", AV_DICT_MATCH_CASE);
			av_opt_set(cctx->priv_data, "b:v", "0", AV_OPT_SEARCH_CHILDREN);
			av_opt_set(cctx, "b:v", "0", AV_OPT_SEARCH_CHILDREN);
			av_dict_set(&av_dict_opts, "b:v", "0", AV_DICT_MATCH_CASE);*/

			/*av_opt_set(cctx->priv_data, "preset", "0", AV_OPT_SEARCH_CHILDREN);
			av_opt_set(cctx, "preset", "0", AV_OPT_SEARCH_CHILDREN);
			av_dict_set(&av_dict_opts, "preset", "0", AV_DICT_MATCH_CASE);*/

			/*av_opt_set(cctx->priv_data, "cpu-used", threadCount.c_str(), AV_OPT_SEARCH_CHILDREN);
			av_opt_set(cctx, "cpu-used", threadCount.c_str(), AV_OPT_SEARCH_CHILDREN);
			av_dict_set(&av_dict_opts, "cpu-used", threadCount.c_str(), AV_DICT_MATCH_CASE);*/

			av_opt_set(cctx->priv_data, "g", toString(_GOP).c_str(), AV_OPT_SEARCH_CHILDREN);
			av_opt_set(cctx, "g", toString(_GOP).c_str(), AV_OPT_SEARCH_CHILDREN);
			av_dict_set(&av_dict_opts, "g", toString(_GOP).c_str(), AV_DICT_MATCH_CASE);
		}
		else if (videoStream->codecpar->codec_id == AV_CODEC_ID_FFV1)
		{

			cctx->gop_size = _GOP;
			cctx->time_base = {1, fps};
			cctx->pix_fmt = file_format;
			cctx->width = width;
			cctx->height = height;
			av_opt_set(cctx->priv_data, "threads", threadCount.c_str(), AV_OPT_SEARCH_CHILDREN);
			av_opt_set(cctx, "threads", threadCount.c_str(), AV_OPT_SEARCH_CHILDREN);
			av_dict_set(&av_dict_opts, "threads", threadCount.c_str(), AV_DICT_MATCH_CASE);

			av_opt_set(cctx->priv_data, "coder", "2", AV_OPT_SEARCH_CHILDREN);
			av_opt_set(cctx, "coder", "2", AV_OPT_SEARCH_CHILDREN);
			av_dict_set(&av_dict_opts, "coder", "2", AV_DICT_MATCH_CASE);

			av_opt_set(cctx->priv_data, "context", "1", AV_OPT_SEARCH_CHILDREN);
			av_opt_set(cctx, "context", "1", AV_OPT_SEARCH_CHILDREN);
			av_dict_set(&av_dict_opts, "context", "1", AV_DICT_MATCH_CASE);

			av_opt_set(cctx->priv_data, "level", "3", AV_OPT_SEARCH_CHILDREN);
			av_opt_set(cctx, "level", "3", AV_OPT_SEARCH_CHILDREN);
			av_dict_set(&av_dict_opts, "level", "3", AV_DICT_MATCH_CASE);

			/*av_opt_set(cctx->priv_data, "g","0", AV_OPT_SEARCH_CHILDREN);
			av_opt_set(cctx, "g", "0", AV_OPT_SEARCH_CHILDREN);
			av_dict_set(&av_dict_opts, "g", "0", AV_DICT_MATCH_CASE);*/
		}
		else if (videoStream->codecpar->codec_id == AV_CODEC_ID_H264)
		{ 

			avcodec_parameters_to_context(cctx, videoStream->codecpar);
			cctx->time_base = {1, fps};
			cctx->pix_fmt = file_format;
			cctx->width = width;
			cctx->height = height;

			// cctx->thread_type = FF_THREAD_SLICE;
			//av_opt_set(cctx->priv_data, "x264opts", "opencl", AV_OPT_SEARCH_CHILDREN);
			//av_opt_set(cctx, "x264opts", "opencl", AV_OPT_SEARCH_CHILDREN);

			//av_opt_set(cctx->priv_data, "preset", preset, AV_OPT_SEARCH_CHILDREN);
			//av_opt_set(cctx, "preset", preset, AV_OPT_SEARCH_CHILDREN);

			av_opt_set(cctx->priv_data, "preset", "p1", AV_OPT_SEARCH_CHILDREN);
			av_opt_set(cctx, "preset", "p1", AV_OPT_SEARCH_CHILDREN);
			av_opt_set(cctx->priv_data, "tune", "lossless", AV_OPT_SEARCH_CHILDREN);
			av_opt_set(cctx, "tune", "lossless", AV_OPT_SEARCH_CHILDREN);
			//av_opt_set(cctx->priv_data, "profile", "main", AV_OPT_SEARCH_CHILDREN);
			//av_opt_set(cctx, "profile", "main", AV_OPT_SEARCH_CHILDREN);

			//av_opt_set(cctx->priv_data, "profile", "high444", AV_OPT_SEARCH_CHILDREN);
			//av_opt_set(cctx, "profile", "high444", AV_OPT_SEARCH_CHILDREN);

			av_opt_set(cctx->priv_data, "crf", "0", AV_OPT_SEARCH_CHILDREN);
			av_opt_set(cctx->priv_data, "qp", "0", AV_OPT_SEARCH_CHILDREN);
			
			av_opt_set(cctx, "crf", "0", AV_OPT_SEARCH_CHILDREN);
			av_opt_set(cctx, "qp", "0", AV_OPT_SEARCH_CHILDREN);
			

			//av_opt_set(cctx->priv_data, "threads", threadCount.c_str(), AV_OPT_SEARCH_CHILDREN);
			//av_opt_set(cctx, "threads", threadCount.c_str(), AV_OPT_SEARCH_CHILDREN);

			if (slices < 1)
				slices = 1;
			else if (slices > 8)
				slices = 8;
			if (slices != 1)
			{
				std::string _slices = toString(slices);
				av_opt_set(cctx->priv_data, "slices", _slices.c_str(), AV_OPT_SEARCH_CHILDREN);
				av_opt_set(cctx, "slices", _slices.c_str(), AV_OPT_SEARCH_CHILDREN);
			}

			if (ofctx->oformat->flags & AVFMT_GLOBALHEADER)
			{
				cctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
			}
			avcodec_parameters_from_context(videoStream->codecpar, cctx);
		}
		else if (videoStream->codecpar->codec_id == AV_CODEC_ID_H265)
		{

			av_opt_set(cctx->priv_data, "preset", preset, AV_OPT_SEARCH_CHILDREN); /// hq ll lossless losslesshq
			av_opt_set(cctx, "preset", preset, AV_OPT_SEARCH_CHILDREN);			   /// hq ll lossless losslesshq
			av_dict_set(&av_dict_opts, "preset", preset, AV_DICT_MATCH_CASE);

			av_opt_set(cctx->priv_data, "lossless", "1", AV_OPT_SEARCH_CHILDREN); /// hq ll lossless losslesshq

			if (slices < 1)
				slices = 1;
			else if (slices > 8)
				slices = 8;
			if (slices != 1)
			{
				std::string _slices = toString(slices);
				av_opt_set(cctx->priv_data, "slices", _slices.c_str(), AV_OPT_SEARCH_CHILDREN);
				av_opt_set(cctx, "slices", _slices.c_str(), AV_OPT_SEARCH_CHILDREN);
			}

			if (kvazaar)
			{
				std::string pr = preset;
				std::string gop = toString(_GOP);
				// av_opt_set(cctx->priv_data, "kvazaar-params", ("lossless=1,preset=" + pr + ",gop="+gop+",threads=" + threadCount).c_str(), AV_OPT_SEARCH_CHILDREN); //limit to 4 threads
				// av_opt_set(cctx, "kvazaar-params", ("lossless=1,preset=" + pr + ",gop="+gop+",threads=" + threadCount).c_str(), AV_OPT_SEARCH_CHILDREN); //limit to 4 threads

				// For unkown reason, not setting a preset provides the best ratio rate/speed
				av_opt_set(cctx->priv_data, "kvazaar-params", ("lossless=1,gop=" + gop + ",threads=" + threadCount).c_str(), AV_OPT_SEARCH_CHILDREN); // limit to 4 threads
				av_opt_set(cctx, "kvazaar-params", ("lossless=1,gop=" + gop + ",threads=" + threadCount).c_str(), AV_OPT_SEARCH_CHILDREN);			  // limit to 4 threads

				width = (int)(ceil(width / 8.) * 8);
				height = (int)(ceil(height / 8.) * 8);
				if (height < frame_height * 2)
					height *= 2;
				videoStream->codecpar->width = width;
				videoStream->codecpar->height = height;
				cctx->gop_size = _GOP;
				file_format = AV_PIX_FMT_YUV420P;
				videoStream->codecpar->format = file_format;
			}
			else
			{
				const std::string x265_params = "lossless=1:threads=1:pools=" + threadCount;
				av_opt_set(cctx->priv_data, "x265_params", x265_params.c_str(), AV_OPT_SEARCH_CHILDREN); // limit to 4 threads

				// #ifdef __unix__
				av_opt_set(cctx->priv_data, "profile", "main444-8", AV_OPT_SEARCH_CHILDREN);
				/*#else
						av_opt_set(cctx->priv_data, "profile", "high", AV_OPT_SEARCH_CHILDREN);
				#endif*/
				av_opt_set(cctx->priv_data, "crf", "0", AV_OPT_SEARCH_CHILDREN);

				av_opt_set(cctx->priv_data, "threads", "1", AV_OPT_SEARCH_CHILDREN);
				av_opt_set(cctx, "threads", "1", AV_OPT_SEARCH_CHILDREN);
				av_dict_set(&av_dict_opts, "threads", "1", AV_DICT_MATCH_CASE);

				av_opt_set(cctx->priv_data, "pools", threadCount.c_str(), AV_OPT_SEARCH_CHILDREN);
				av_opt_set(cctx, "pools", threadCount.c_str(), AV_OPT_SEARCH_CHILDREN);
				av_dict_set(&av_dict_opts, "pools", threadCount.c_str(), AV_DICT_MATCH_CASE);
			}

			cctx->pix_fmt = file_format;
			cctx->width = width;
			cctx->height = height;
		}
		else if (videoStream->codecpar->codec_id == AV_CODEC_ID_VP9 || videoStream->codecpar->codec_id == AV_CODEC_ID_VP8)
		{
			int speed = presetToLevel(preset);
			if (speed < 0)
				speed = 0;
			else if (speed > 8)
				speed = 8;
			std::string _speed = toString(speed);
			av_opt_set(cctx->priv_data, "speed", _speed.c_str(), AV_OPT_SEARCH_CHILDREN); /// hq ll lossless losslesshq
			av_opt_set(cctx, "speed", _speed.c_str(), AV_OPT_SEARCH_CHILDREN);			  /// hq ll lossless losslesshq
			av_dict_set(&av_dict_opts, "speed", _speed.c_str(), AV_OPT_SEARCH_CHILDREN);

			/*av_opt_set(cctx->priv_data, "maxrate", "200k", AV_OPT_SEARCH_CHILDREN); /// hq ll lossless losslesshq
			av_opt_set(cctx, "maxrate", "200k", AV_OPT_SEARCH_CHILDREN); /// hq ll lossless losslesshq
			av_dict_set(&av_dict_opts, "maxrate", "200k", AV_OPT_SEARCH_CHILDREN);
			av_opt_set(cctx->priv_data, "minrate", "200k", AV_OPT_SEARCH_CHILDREN); /// hq ll lossless losslesshq
			av_opt_set(cctx, "minrate", "200k", AV_OPT_SEARCH_CHILDREN); /// hq ll lossless losslesshq
			av_dict_set(&av_dict_opts, "minrate", "200k", AV_OPT_SEARCH_CHILDREN);*/
			av_opt_set(cctx->priv_data, "b", "0", AV_OPT_SEARCH_CHILDREN); /// hq ll lossless losslesshq
			av_opt_set(cctx, "b", "0", AV_OPT_SEARCH_CHILDREN);			   /// hq ll lossless losslesshq
			av_dict_set(&av_dict_opts, "b", "0", AV_OPT_SEARCH_CHILDREN);  /// hq ll lossless losslesshq

			av_opt_set(cctx->priv_data, "threads", threadCount.c_str(), AV_OPT_SEARCH_CHILDREN); /// hq ll lossless losslesshq
			av_opt_set(cctx, "threads", threadCount.c_str(), AV_OPT_SEARCH_CHILDREN);			 /// hq ll lossless losslesshq
			av_dict_set(&av_dict_opts, "threads", threadCount.c_str(), AV_OPT_SEARCH_CHILDREN);	 /// hq ll lossless losslesshq

			av_opt_set(cctx->priv_data, "lossless", "1", AV_OPT_SEARCH_CHILDREN); /// hq ll lossless losslesshq
			av_opt_set(cctx, "lossless", "1", AV_OPT_SEARCH_CHILDREN);			  /// hq ll lossless losslesshq
			av_dict_set(&av_dict_opts, "lossless", "1", AV_OPT_SEARCH_CHILDREN);  /// hq ll lossless losslesshq

			av_opt_set(cctx->priv_data, "row-mt", "1", AV_OPT_SEARCH_CHILDREN); /// hq ll lossless losslesshq
			av_opt_set(cctx, "row-mt", "1", AV_OPT_SEARCH_CHILDREN);			/// hq ll lossless losslesshq
			av_dict_set(&av_dict_opts, "row-mt", "1", AV_OPT_SEARCH_CHILDREN);	/// hq ll lossless losslesshq

			// av_opt_set(cctx->priv_data, "cpu-used", "0", AV_OPT_SEARCH_CHILDREN); /// hq ll lossless losslesshq
			// av_opt_set(cctx, "cpu-used", "0", AV_OPT_SEARCH_CHILDREN); /// hq ll lossless losslesshq
			// av_dict_set(&av_dict_opts, "cpu-used", "0", AV_OPT_SEARCH_CHILDREN); /// hq ll lossless losslesshq

			// av_opt_set(cctx->priv_data, "quality", "good", AV_OPT_SEARCH_CHILDREN); /// hq ll lossless losslesshq
			// av_opt_set(cctx, "quality", "good", AV_OPT_SEARCH_CHILDREN); /// hq ll lossless losslesshq
			// av_dict_set(&av_dict_opts, "good", "best", AV_OPT_SEARCH_CHILDREN); /// hq ll lossless losslesshq

			if (videoStream->codecpar->codec_id == AV_CODEC_ID_VP9)
			{
				const std::string vp9_params = "row-mt=1:quality=good:speed=0:lossless=1:threads=" + threadCount + ":g=" + toString(_GOP) + ":cpu-used=" + threadCount;
				av_opt_set(cctx->priv_data, "libvpx-vp9", vp9_params.c_str(), AV_OPT_SEARCH_CHILDREN); // limit to 4 threads
			}
			else
			{
				av_opt_set(cctx->priv_data, "crf", "4", AV_OPT_SEARCH_CHILDREN); /// hq ll lossless losslesshq
				av_opt_set(cctx, "crf", "4", AV_OPT_SEARCH_CHILDREN);			 /// hq ll lossless losslesshq
				av_dict_set(&av_dict_opts, "crf", "4", AV_OPT_SEARCH_CHILDREN);	 /// hq ll lossless losslesshq

				const std::string vp8_params = "crf=4";											   // "good::lossless=1:threads=" + threadCount + ":g=" + toString(_GOP) + ":cpu-used=" + threadCount;
				av_opt_set(cctx->priv_data, "libvpx", vp8_params.c_str(), AV_OPT_SEARCH_CHILDREN); // limit to 4 threads
				file_format = AV_PIX_FMT_YUV420P;
				if (height < frame_height * 2)
					height *= 2;
				videoStream->codecpar->width = width;
				videoStream->codecpar->height = height;
			}

			av_opt_set(cctx->priv_data, "threads", threadCount.c_str(), AV_OPT_SEARCH_CHILDREN);
			av_opt_set(cctx, "threads", threadCount.c_str(), AV_OPT_SEARCH_CHILDREN);
			av_dict_set(&av_dict_opts, "threads", threadCount.c_str(), AV_DICT_MATCH_CASE);

			cctx->gop_size = _GOP;
			cctx->pix_fmt = file_format;
			cctx->width = width;
			cctx->height = height;
		}
		else if (videoStream->codecpar->codec_id == AV_CODEC_ID_DIRAC)
		{

			av_opt_set(cctx->priv_data, "lossless", "1", AV_OPT_SEARCH_CHILDREN); /// hq ll lossless losslesshq
			av_opt_set(cctx, "lossless", "1", AV_OPT_SEARCH_CHILDREN);			  /// hq ll lossless losslesshq
			av_dict_set(&av_dict_opts, "lossless", "1", AV_OPT_SEARCH_CHILDREN);  /// hq ll lossless losslesshq

			av_opt_set(cctx->priv_data, "b", "1000000000", AV_OPT_SEARCH_CHILDREN); /// hq ll lossless losslesshq
			av_opt_set(cctx, "b", "1000000000", AV_OPT_SEARCH_CHILDREN);			/// hq ll lossless losslesshq
			av_dict_set(&av_dict_opts, "b", "1000000000", AV_OPT_SEARCH_CHILDREN);	/// hq ll lossless losslesshq

			const std::string dirac = "b=1000000000:lossless=1:threads=" + threadCount;
			av_opt_set(cctx->priv_data, "vp2", dirac.c_str(), AV_OPT_SEARCH_CHILDREN); // limit to 4 threads
			// #ifdef __unix__
			av_opt_set(cctx->priv_data, "profile", "main444-8", AV_OPT_SEARCH_CHILDREN);

			// #ifdef __unix__
			av_opt_set(cctx->priv_data, "threads", threadCount.c_str(), AV_OPT_SEARCH_CHILDREN);
			av_opt_set(cctx, "threads", threadCount.c_str(), AV_OPT_SEARCH_CHILDREN);
			av_dict_set(&av_dict_opts, "threads", threadCount.c_str(), AV_DICT_MATCH_CASE);

			// cctx->gop_size = _GOP;
			cctx->pix_fmt = file_format;
			cctx->width = width;
			cctx->height = height;
		}

		lastKeyFrame = 0;
		GOP = _GOP;

		// write the video size as comment (used when encoding with kvazaar HEVC encoder)
		av_dict_set(&ofctx->metadata, "comment", ("size:" + toString(frame_width) + "x" + toString(frame_height)).c_str(), 0);

		if ((err = avcodec_open2(cctx, codec, &av_dict_opts)) < 0)
		{
			printf("Failed to open codec %i\n", err);
			Free();
			return false;
		}

		if (!(oformat->flags & AVFMT_NOFILE))
		{
			if ((err = avio_open(&ofctx->pb, tmp_name.c_str(), AVIO_FLAG_WRITE)) < 0)
			{
				RIR_LOG_ERROR("Failed to open file %s", tmp_name.c_str());
				Free();
				return false;
			}
		}

		if ((err = avformat_write_header(ofctx, NULL)) < 0)
		{
			RIR_LOG_ERROR("Failed to write header in file %s", tmp_name.c_str());
			Free();
			return false;
		}

		av_dump_format(ofctx, 0, tmp_name.c_str(), 1);
		return true;
	}

	bool H264Capture::AddFrame(const unsigned short *img, bool key)
	{

		/**
		If imgT is not NULL, image is saved in temperature.
		The V component will hold the TI values
		*/
		// const VipNDArray tmp = ar.shape() == vipVector(cctx->height, cctx->width) ? ar : ar.resize(vipVector(cctx->height, cctx->width));
		const unsigned short *data = img; //(const unsigned short*)tmp.constData();

		int err;
		if (!videoFrame)
		{

			videoFrame = av_frame_alloc();
			videoFrame->format = file_format;
			videoFrame->width = cctx->width;
			videoFrame->height = cctx->height;

			if ((err = av_frame_get_buffer(videoFrame, 32)) < 0)
			{
				RIR_LOG_ERROR("Failed to allocate picture with format %i, w = %i, h = %i", (int)file_format, cctx->width, cctx->height);
				return false;
			}
		}

		if (!kvazaar && (videoStream->codecpar->codec_id != AV_CODEC_ID_FFV1 && videoStream->codecpar->codec_id != AV_CODEC_ID_VP9))
		{
			if (frameCounter == 0)
				key = true;
			else if (frameCounter - lastKeyFrame >= GOP)
				key = true;

			if (key)
			{
				lastKeyFrame = frameCounter;
				videoFrame->pict_type = AV_PICTURE_TYPE_I;
			}
			else
				videoFrame->pict_type = AV_PICTURE_TYPE_NONE;
		}

		if (cctx->pix_fmt == AV_PIX_FMT_YUV444P)
		{

			for (int y = 0; y < cctx->height; ++y)
			{
				unsigned char *d0 = videoFrame->data[0] + y * videoFrame->linesize[0];
				unsigned char *d1 = videoFrame->data[1] + y * videoFrame->linesize[1];
				unsigned char *d2 = videoFrame->data[2] + y * videoFrame->linesize[2];
				for (int i = 0; i < cctx->width; ++i)
				{
					unsigned short v = data[i + y * cctx->width];
					d1[i] = v & 0xFF;
					d2[i] = v >> 8;
					d0[i] = 0;
				}
			}
		}
		else
		{ // AV_PIX_FMT_YUV420P
			/*memset(videoFrame->data[0],0,cctx->height * videoFrame->linesize[0]);
			memset(videoFrame->data[1],0, cctx->height * videoFrame->linesize[1]);
			memset(videoFrame->data[2],0, cctx->height * videoFrame->linesize[2]);*/

			memset(videoFrame->buf[0]->data, 0, videoFrame->buf[0]->size);

			for (int y = 0; y < frame_height; ++y)
			{
				unsigned char *d0 = videoFrame->data[0] + y * videoFrame->linesize[0];
				unsigned char *d0_2 = videoFrame->data[0] + (frame_height + y) * videoFrame->linesize[0];

				for (int i = 0; i < frame_width; ++i)
				{
					unsigned short v = data[i + y * frame_width];
					d0[i] = v & 0xFF;
					d0_2[i] = v >> 8;
				}
			}
		}

		videoFrame->pts = frameCounter;		// *videoStream->time_base.den;
		videoFrame->pkt_dts = frameCounter; // *videoStream->time_base.den;
		videoFrame->pts = frameCounter;		// *videoStream->time_base.den;
		videoFrame->duration = 1;			// videoStream->time_base.den;
		// videoFrame->pkt_pos = frameCounter;
		frameCounter++;

		if ((err = avcodec_send_frame(cctx, videoFrame)) < 0)
		{
			RIR_LOG_ERROR("Failed to send frame (%i)", err);
			return false;
		}

		AVPacket pkt;
		av_init_packet(&pkt);
		pkt.data = NULL;
		pkt.size = 0;

		if (avcodec_receive_packet(cctx, &pkt) == 0)
		{
			// pkt.flags |= AV_PKT_FLAG_KEY;
			pkt.duration = 1;
			av_interleaved_write_frame(ofctx, &pkt);
			av_packet_unref(&pkt);
		}
		return true;
	}

	bool H264Capture::AddFrame(const unsigned short *img, const unsigned char *IT, bool key)
	{

		/**
		If imgT is not NULL, image is saved in temperature.
		The V component will hold the TI values
		*/
		// const VipNDArray tmp = ar.shape() == vipVector(cctx->height, cctx->width) ? ar : ar.resize(vipVector(cctx->height, cctx->width));
		const unsigned short *data = img; //(const unsigned short*)tmp.constData();

		int err;
		if (!videoFrame)
		{

			videoFrame = av_frame_alloc();
			videoFrame->format = file_format;
			videoFrame->width = cctx->width;
			videoFrame->height = cctx->height;

			if ((err = av_frame_get_buffer(videoFrame, 32)) < 0)
			{
				RIR_LOG_ERROR("Failed to allocate picture with format %i, w = %i, h = %i", (int)file_format, cctx->width, cctx->height);
				return false;
			}
		}

		if (!kvazaar && (videoStream->codecpar->codec_id != AV_CODEC_ID_FFV1 && videoStream->codecpar->codec_id != AV_CODEC_ID_VP9))
		{
			if (frameCounter == 0)
				key = true;
			else if (frameCounter - lastKeyFrame > GOP)
				key = true;

			if (key)
			{
				lastKeyFrame = frameCounter;
				videoFrame->pict_type = AV_PICTURE_TYPE_I;
			}
			else
				videoFrame->pict_type = AV_PICTURE_TYPE_NONE;
		}

		if (cctx->pix_fmt == AV_PIX_FMT_YUV444P)
		{
			for (int y = 0; y < cctx->height; ++y)
			{
				unsigned char *d0 = videoFrame->data[0] + y * videoFrame->linesize[0];
				unsigned char *d1 = videoFrame->data[1] + y * videoFrame->linesize[1];
				unsigned char *d2 = videoFrame->data[2] + y * videoFrame->linesize[2];
				for (int i = 0; i < cctx->width; ++i)
				{
					unsigned short v = data[i + y * cctx->width];
					d0[i] = IT[i + y * cctx->width];
					d1[i] = v & 0xFF;
					d2[i] = v >> 8;
				}
			}
		}
		else
		{ // AV_PIX_FMT_YUV420P
			memset(videoFrame->buf[0]->data, 0, videoFrame->buf[0]->size);

			for (int y = 0; y < frame_height; ++y)
			{
				unsigned char *d0 = videoFrame->data[0] + y * videoFrame->linesize[0];
				unsigned char *d0_2 = videoFrame->data[0] + (frame_height + y) * videoFrame->linesize[0];
				unsigned char *d1 = videoFrame->data[1] + y * videoFrame->linesize[1];
				for (int i = 0; i < frame_width; ++i)
				{
					unsigned short v = data[i + y * frame_width];
					d0[i] = v & 0xFF;
					d0_2[i] = v >> 8;
					d1[i] = IT[i + y * frame_width];
				}
			}
		}

		videoFrame->pts = frameCounter;		// *videoStream->time_base.den;
		videoFrame->pkt_dts = frameCounter; // *videoStream->time_base.den;
		videoFrame->pts = frameCounter;		// *videoStream->time_base.den;
		videoFrame->duration = 1;			// videoStream->time_base.den;
		// videoFrame->pkt_pos = frameCounter;
		frameCounter++;

		if ((err = avcodec_send_frame(cctx, videoFrame)) < 0)
		{
			RIR_LOG_ERROR("Failed to send frame (%i)", err);
			return false;
		}

		AVPacket pkt;
		av_init_packet(&pkt);
		pkt.data = NULL;
		pkt.size = 0;

		if (avcodec_receive_packet(cctx, &pkt) == 0)
		{
			pkt.flags |= AV_PKT_FLAG_KEY;
			pkt.duration = 1;
			av_interleaved_write_frame(ofctx, &pkt);
			av_packet_unref(&pkt);
		}
		return true;
	}

	bool H264Capture::Finish()
	{

		// DELAYED FRAMES
		AVPacket pkt;
		av_init_packet(&pkt);
		pkt.data = NULL;
		pkt.size = 0;

		for (;;)
		{
			avcodec_send_frame(cctx, NULL);
			if (avcodec_receive_packet(cctx, &pkt) == 0)
			{
				av_interleaved_write_frame(ofctx, &pkt);
				av_packet_unref(&pkt);
			}
			else
			{
				break;
			}
		}

		av_write_trailer(ofctx);
		if (!(oformat->flags & AVFMT_NOFILE))
		{
			int err = avio_close(ofctx->pb);
			if (err < 0)
			{
				RIR_LOG_ERROR("Failed to close file (%i)", err);
			}
		}

		Free();
		if (codec_name == "h264" || codec_name == "h265")
			return Remux();
		else
		{
			if (file_exists(fname.c_str()))
			{
				remove(fname.c_str());
			}
			int res = rename(tmp_name.c_str(), fname.c_str());
			return res == 0;
		}
	}

	void H264Capture::Free()
	{
		if (videoFrame)
		{
			av_frame_free(&videoFrame);
			videoFrame = NULL;
		}
		if (cctx)
		{
			avcodec_free_context(&cctx);
		}
		if (ofctx)
		{
			avformat_free_context(ofctx);
			ofctx = NULL;
		}
		if (swsCtx)
		{
			sws_freeContext(swsCtx);
			swsCtx = NULL;
		}
	}

	bool H264Capture::Remux()
	{

		// int r =rename(tmp_name.c_str(), fname.c_str());
		// return true;

		AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
		int err;
		bool res = true;
		AVPacket videoPkt;
		int ts = 0;
		AVStream *inVideoStream = 0;
		AVStream *outVideoStream = 0;
		const AVCodec *c = 0;
		std::string outtmp = fname + ".mp4";

		if ((err = avformat_open_input(&ifmt_ctx, tmp_name.c_str(), 0, 0)) < 0)
		{
			RIR_LOG_ERROR("Failed to open input file for remuxing (%i)", err);
			res = false;
			goto end;
		}
		if ((err = avformat_find_stream_info(ifmt_ctx, 0)) < 0)
		{
			RIR_LOG_ERROR("Failed to retrieve input stream information (%i)", err);
			res = false;
			goto end;
		}

		/*if (ifmt_ctx->streams[0]->codec->codec_id == AV_CODEC_ID_HEVC) {
			AVOutputFormat* f = av_oformat_next(NULL);
			while (f) {
				if (f->video_codec == AV_CODEC_ID_HEVC) {
					bool stop = true;
				}
				f = av_oformat_next(f);
			}
		}*/

		if ((err = avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, outtmp.c_str())))
		{
			RIR_LOG_ERROR("Failed to allocate output context (%i)", err);
			res = false;
			goto end;
		}

		// write the video size as comment (used when encoding with kvazaar HEVC encoder)
		av_dict_set(&ofmt_ctx->metadata, "comment", ("size:" + toString(frame_width) + "x" + toString(frame_height)).c_str(), 0);

		c = avcodec_find_encoder(AV_CODEC_ID_HEVC);

		inVideoStream = ifmt_ctx->streams[0];
		outVideoStream = avformat_new_stream(ofmt_ctx, c);
		if (!outVideoStream)
		{
			RIR_LOG_ERROR("Failed to allocate output video stream");
			res = false;
			goto end;
		}
		outVideoStream->time_base = {1, fps};
		avcodec_parameters_copy(outVideoStream->codecpar, inVideoStream->codecpar);

		outVideoStream->codecpar->codec_tag = 0;

		if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
		{
			if ((err = avio_open(&ofmt_ctx->pb, outtmp.c_str(), AVIO_FLAG_WRITE)) < 0)
			{
				RIR_LOG_ERROR("Failed to open output file (%i)", err);
				res = false;
				goto end;
			}
		}
		if ((err = avformat_write_header(ofmt_ctx, 0)) < 0)
		{
			RIR_LOG_ERROR("Failed to write header to output file (%i)", err);
			res = false;
			goto end;
		}

		while (true)
		{
			if ((err = av_read_frame(ifmt_ctx, &videoPkt)) < 0)
			{
				break;
			}
			videoPkt.stream_index = outVideoStream->index;
			videoPkt.pts = ts;
			videoPkt.dts = ts;
			videoPkt.duration = 12800; // av_rescale_q(videoPkt.duration, inVideoStream->time_base, outVideoStream->time_base);
			ts += (int)videoPkt.duration;
			videoPkt.pos = (ts / 12800) - 1; //-1;

			if ((err = av_interleaved_write_frame(ofmt_ctx, &videoPkt)) < 0)
			{
				RIR_LOG_ERROR("Failed to mux packet (%i)", err);
				av_packet_unref(&videoPkt);
				break;
			}
			av_packet_unref(&videoPkt);
		}

		av_write_trailer(ofmt_ctx);

	end:
		if (ifmt_ctx)
		{
			avformat_close_input(&ifmt_ctx);
		}
		if (ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
		{
			avio_closep(&ofmt_ctx->pb);
		}
		if (ofmt_ctx)
		{
			avformat_free_context(ofmt_ctx);
		}
		if (remove(tmp_name.c_str()) != 0)
			res = false;
		if (file_exists(fname.c_str()))
		{
			if (remove(fname.c_str()) != 0)
				res = false;
		}
		if (rename(outtmp.c_str(), fname.c_str()) != 0)
			res = false;
		/*else if (remove(outtmp.c_str()) != 0)
		res = false;*/

		return res;
	}

	struct RunningAverage
	{
		int width;
		int height;
		int image_len;
		int max_size; // max number of images
		int size;	  // current number of images
		int insert_pos;
		std::vector<unsigned short> images;

		RunningAverage()
			: width(0), height(0), image_len(0), max_size(0), size(0), insert_pos(0)
		{
		}
		void reset(int w, int h, int max_images)
		{
			if (w == width && h == height && max_images == max_size)
			{
				size = 0;
				insert_pos = 0;
				return;
			}
			width = w;
			height = h;
			image_len = w * h;
			max_size = max_images;
			size = 0;
			insert_pos = 0;
			images.resize(w * h * max_images);
		}
		void addImage(const unsigned short *img, int threads = 1)
		{
			if (size < max_size)
			{
				memcpy(images.data() + size * width * height, img, width * height * 2);
				++size;
			}
			else
			{
				// move last images to beginning
				// memmove(images.data(), images.data() + width*height , (max_size - 1)*width*height * 2);
				// copy image at the right location
				memcpy(images.data() + (insert_pos)*width * height, img, width * height * 2);
				insert_pos++;
				if (insert_pos >= max_size)
					insert_pos = 0;
			}
		}
		unsigned short pixel(int x, int y) const
		{
			int sum = 0;
			int index = x + y * width;
			for (int i = 0; i < size; ++i)
			{
				sum += images[i * image_len + index];
			}
			return size == 32 ? sum >> 5 : sum / size;
		}
		unsigned short pixel(int index) const
		{
			int sum = 0;
			for (int i = 0; i < size; ++i)
			{
				sum += images[i * image_len + index];
			}
			return size == 32 ? sum >> 5 : sum / size;
		}
		void resetPixel(int x, int y, unsigned short value)
		{
			int index = x + y * width;
			for (int i = 0; i < size; ++i)
			{
				images[i * image_len + index] = value;
			}
		}
		void resetPixel(int index, unsigned short value)
		{
			for (int i = 0; i < size; ++i)
			{
				images[i * image_len + index] = value;
			}
		}
	};

	struct RunningAverage2
	{
		struct Const
		{
			unsigned short value;
			short count;
			Const() : value(0), count(0) {}
		};
		size_t width;
		size_t height;
		size_t image_len;
		size_t max_size; // max number of images

		std::vector<std::vector<unsigned short>> images;
		std::vector<unsigned> sums;
		std::vector<Const> consts;

		RunningAverage2()
			: width(0), height(0), image_len(0), max_size(0)
		{
		}
		void reset(size_t w, size_t h, size_t max_images)
		{

			width = w;
			height = h;
			image_len = w * h;
			max_size = max_images;
			sums.resize(image_len);
			std::fill_n(sums.begin(), sums.size(), 0);
			consts.resize(image_len);
			std::fill_n(consts.begin(), consts.size(), Const());
		}
		void addImage(const unsigned short *img, int threads = 1)
		{

			// update sums
#pragma omp parallel for num_threads(threads)
			for (int i = 0; i < (int)image_len; ++i)
			{
				// add last image
				sums[i] += img[i];

				if (images.size() == max_size)
				{
					// remove first image
					if (consts[i].count)
					{
						--consts[i].count;
						sums[i] -= consts[i].value;
					}
					else if (images.size() > 0)
						sums[i] -= images[0][i];
				}
			}

			// update images
			if (images.size() < max_size)
			{
				images.push_back(std::vector<unsigned short>(img, img + image_len));
			}
			else
			{
				std::vector<unsigned short> tmp = std::move(images[0]);
				std::copy(img, img + image_len, tmp.begin());
				images.erase(images.begin());
				images.push_back(std::move(tmp));
			}
		}
		unsigned short pixel(size_t x, size_t y) const
		{
			size_t index = x + y * width;
			return pixel(index);
		}
		unsigned short pixel(size_t index) const
		{
			return sums[index] / images.size();
		}
		void resetPixel(size_t x, size_t y, unsigned short value)
		{
			size_t index = x + y * width;
			resetPixel(index, value);
		}
		void resetPixel(size_t index, unsigned short value)
		{
			consts[index].value = value;
			consts[index].count = images.size();
			sums[index] = (unsigned)value * images.size();
		}
	};

	class H264_Saver::PrivateData
	{
	public:
		H264Capture *encoder;
		FileAttributes attributes;
		int compressionLevel;
		int lowValueError;
		int highValueError;
		int width, height, stop_lossy_height;
		int fps;
		int keyCount;
		size_t frameCount;
		int GOP;
		int threads;
		int slices;
		int smartSmooth;
		int inputCamera;
		unsigned short min;
		std::string codec;
		BadPixels bp;
		bool bp_enabled;
		bool subtractMin;
		bool subtractLocalMin;
		std::multiset<double> diffs;
		std::vector<unsigned short> lastDL;
		std::vector<std::pair<double, double>> firstStdDevs;
		std::vector<std::pair<double, double>> stdDevs;
		double meanStdDev;
		double stdFactor;
		std::vector<unsigned short> refT;
		std::vector<unsigned short> prev;
		std::vector<unsigned short> prevT; // usefull?
		std::vector<unsigned short> tmp;
		std::vector<unsigned short> tmpT;
		std::vector<unsigned short> tmpDL;
		std::vector<unsigned char> IT;
		std::vector<unsigned short> localMins;

		std::vector<unsigned short> lowError;
		std::vector<unsigned short> highError;
		// std::vector < std::vector<unsigned short> > cum;
		RunningAverage2 cum;
		unsigned runningAverage;
		unsigned hist[65536 >> 2];

		PrivateData() : encoder(NULL), compressionLevel(0), lowValueError(6), highValueError(2),
						width(0), height(0), stop_lossy_height(0),
						fps(0), keyCount(0), frameCount(0), GOP(50), threads(NUM_THREADS_H264), slices(1), smartSmooth(0), inputCamera(0), bp_enabled(false), subtractMin(false), subtractLocalMin(false), meanStdDev(0),
						stdFactor(5), runningAverage(32) {}
	};

	H264_Saver::H264_Saver()
	{
		m_data = new PrivateData();
		m_data->codec = "h264";
		m_data->GOP = 50;
	}

	H264_Saver::~H264_Saver()
	{
		close();
		delete m_data;
	}

	void H264_Saver::setCompressionLevel(int clevel)
	{
		m_data->compressionLevel = clevel;
		;
	}
	int H264_Saver::compressionLevel() const
	{
		return m_data->compressionLevel;
	}

	void H264_Saver::setLowValueError(int max_error_T)
	{
		m_data->lowValueError = max_error_T;
	}
	int H264_Saver::lowValueError() const
	{
		return m_data->lowValueError;
	}

	void H264_Saver::setHighValueError(int max_error_T)
	{
		m_data->highValueError = max_error_T;
	}
	int H264_Saver::highValueError() const
	{
		return m_data->highValueError;
	}

	bool H264_Saver::setParameter(const char *key, const char *value)
	{
		if (strcmp(key, "lowValueError") == 0)
		{
			setLowValueError(fromString<int>(value));
			return true;
		}
		else if (strcmp(key, "highValueError") == 0)
		{
			setHighValueError(fromString<int>(value));
			return true;
		}
		else if (strcmp(key, "compressionLevel") == 0)
		{
			setCompressionLevel(fromString<int>(value));
			return true;
		}
		else if (strcmp(key, "codec") == 0)
		{
			m_data->codec = value;
			return true;
		}
		else if (strcmp(key, "GOP") == 0)
		{
			m_data->GOP = fromString<int>(value);
			return true;
		}
		else if (strcmp(key, "threads") == 0)
		{
			m_data->threads = fromString<int>(value);
			return true;
		}
		else if (strcmp(key, "slices") == 0)
		{
			m_data->slices = fromString<int>(value);
			return true;
		}
		else if (strcmp(key, "stdFactor") == 0)
		{
			m_data->stdFactor = fromString<double>(value);
			return true;
		}
		else if (strcmp(key, "inputCamera") == 0)
		{
			m_data->inputCamera = fromString<int>(value);
			return true;
		}
		else if (strcmp(key, "removeBadPixels") == 0)
		{
			m_data->bp_enabled = fromString<int>(value) != 0;
			return true;
		}
		else if (strcmp(key, "subtractMin") == 0)
		{
			m_data->subtractMin = fromString<int>(value) != 0;
			return true;
		}
		else if (strcmp(key, "subtractLocalMin") == 0)
		{
			m_data->subtractLocalMin = fromString<int>(value) != 0;
			return true;
		}
		else if (strcmp(key, "runningAverage") == 0)
		{
			m_data->runningAverage = fromString<int>(value);
			if (m_data->runningAverage > 64)
				m_data->runningAverage = 64;
			if (m_data->cum.max_size != (int)m_data->runningAverage && m_data->cum.width > 0)
				m_data->cum.reset(m_data->cum.width, m_data->cum.height, m_data->runningAverage);
			return true;
		}
		return false;
	}

	std::string H264_Saver::getParameter(const char *key) const
	{
		if (strcmp(key, "lowValueError") == 0)
		{
			return toString(lowValueError());
		}
		else if (strcmp(key, "highValueError") == 0)
		{
			return toString(highValueError());
		}
		else if (strcmp(key, "compressionLevel") == 0)
		{
			return toString(compressionLevel());
		}
		else if (strcmp(key, "codec") == 0)
		{
			return m_data->codec;
		}
		else if (strcmp(key, "GOP") == 0)
		{
			return toString(m_data->GOP);
		}
		else if (strcmp(key, "threads") == 0)
		{
			return toString(m_data->threads);
		}
		else if (strcmp(key, "slices") == 0)
		{
			return toString(m_data->slices);
		}
		else if (strcmp(key, "stdFactor") == 0)
		{
			return toString(m_data->stdFactor);
		}
		else if (strcmp(key, "inputCamera") == 0)
		{
			return toString(m_data->inputCamera);
		}
		else if (strcmp(key, "removeBadPixels") == 0)
		{
			return toString((int)m_data->bp_enabled);
		}
		else if (strcmp(key, "subtractMin") == 0)
		{
			return toString((int)m_data->subtractMin);
		}
		else if (strcmp(key, "subtractLocalMin") == 0)
		{
			return toString((int)m_data->subtractLocalMin);
		}
		else if (strcmp(key, "runningAverage") == 0)
		{
			return toString(m_data->runningAverage);
		}
		return std::string();
	}

	bool H264_Saver::isOpen() const
	{
		return m_data->encoder != NULL;
	}

	bool H264_Saver::open(const char *filename, int width, int height, int stop_lossy_height, int fps)
	{
		close();
		if (file_exists(filename) && remove(filename) != 0)
		{
			return false;
		}
		m_data->encoder = new H264Capture();
		if (!m_data->encoder->Init(filename, width, height, fps, compressionToPreset(compressionLevel()), m_data->codec.c_str(), m_data->GOP, m_data->threads, m_data->slices))
		{
			delete m_data->encoder;
			m_data->encoder = NULL;
			return false;
		}
		m_data->width = width;
		m_data->height = height;
		m_data->meanStdDev = 0;
		m_data->fps = fps;
		m_data->frameCount = m_data->keyCount = 0;
		m_data->stop_lossy_height = stop_lossy_height;
		m_data->lastDL.resize(width * height);
		m_data->refT.resize(width * height);
		m_data->prevT.resize(width * height);
		m_data->prev.resize(width * height);
		m_data->tmp.resize(width * height);
		m_data->tmpDL.resize(width * height);
		m_data->tmpT.resize(width * height);
		m_data->IT.resize(width * height);
		m_data->cum.reset(width, stop_lossy_height, m_data->runningAverage);
		m_data->diffs.clear();
		m_data->stdDevs.clear();
		m_data->firstStdDevs.clear();
		m_data->localMins.clear();
		m_data->attributes.open(filename);

		return true;
	}

	void H264_Saver::close()
	{
		if (m_data->encoder)
		{
			m_data->encoder->Finish();
			delete m_data->encoder;
			m_data->encoder = NULL;
			m_data->lastDL.clear();
			m_data->refT.clear();
			m_data->prev.clear();
			m_data->prevT.clear();
			m_data->tmp.clear();
			m_data->diffs.clear();
			m_data->stdDevs.clear();
			m_data->firstStdDevs.clear();
			m_data->localMins.clear();
			m_data->bp_enabled = false;
			m_data->lowError.clear();
			m_data->highError.clear();
			// save trailer
			m_data->attributes.addGlobalAttribute("GOP", toString(m_data->GOP));

			if (m_data->subtractLocalMin)
			{
				std::string mins((const char *)m_data->localMins.data(), m_data->localMins.size() * 2);
				m_data->attributes.addGlobalAttribute("LOCAL_MINS", mins);
			}
			m_data->attributes.close();
		}
	}

	void H264_Saver::addGlobalAttribute(const std::string &key, const std::string &value)
	{
		m_data->attributes.addGlobalAttribute(key, value);
	}
	void H264_Saver::clearGlobalAttributes()
	{
		setGlobalAttributes(std::map<std::string, std::string>());
	}
	void H264_Saver::setGlobalAttributes(const std::map<std::string, std::string> &attributes)
	{
		m_data->attributes.setGlobalAttributes(attributes);
	}
	bool H264_Saver::addImageLossLess(const unsigned short *img, int64_t timestamp_ns, const std::map<std::string, std::string> &attributes, bool is_key)
	{
		if (!isOpen())
			return false;

		if (!m_data->encoder->AddFrame(img, is_key))
			return false;

		++m_data->frameCount;
		m_data->attributes.resize(m_data->frameCount);
		m_data->attributes.setTimestamp(m_data->frameCount - 1, timestamp_ns);
		m_data->attributes.setAttributes(m_data->frameCount - 1, attributes);
		return true;
	}
	bool H264_Saver::addImageLossLess(const unsigned short *img, const unsigned char *IT, int64_t timestamp_ns, const std::map<std::string, std::string> &attributes, bool is_key)
	{
		if (!isOpen())
			return false;

		if (!m_data->encoder->AddFrame(img, IT, is_key))
			return false;

		++m_data->frameCount;
		m_data->attributes.resize(m_data->frameCount);
		m_data->attributes.setTimestamp(m_data->frameCount - 1, timestamp_ns);
		m_data->attributes.setAttributes(m_data->frameCount - 1, attributes);
		return true;
	}

	static unsigned get_background(const unsigned short *im, unsigned *hist, int size, unsigned short *min = NULL)
	{
		memset(hist, 0, (65536 >> 2) * 4);
		const unsigned short *ptr = im;
		if (!min)
		{
			for (int i = 0; i < size; ++i)
				hist[ptr[i] >> 2]++;
		}
		else
		{
			unsigned short m = 65535;
			for (int i = 0; i < size; ++i)
			{
				hist[ptr[i] >> 2]++;
				if (ptr[i] < m)
					m = ptr[i];
			}
		}

		unsigned max = hist[0];
		unsigned index = 0;
		for (int i = 1; i < (65536 >> 2); ++i)
		{
			if (hist[i] > max)
			{
				max = hist[i];
				index = i;
			}
		}
		// compute number of pixels before
		/*int count = 0;
		for (int i = index; i >= 0; --i)
		count += hist[i];
		printf("under back: %f\n", (double)count / size);*/
		return (index << 2) + 1;
	}

	static std::pair<double, double> stdDev(const unsigned short *prev, const unsigned short *img, int width, int height, const unsigned short *img_DL = NULL, unsigned *back = NULL)
	{
		if (!back || !img_DL)
		{
			double sum_diff2 = 0;
			double sum_diff = 0;
			int s = height * width;
			for (int i = 0; i < s; ++i)
			{
				int diff = std::abs(img[i] - prev[i]);
				sum_diff2 += diff * diff;
				sum_diff += diff;
			}
			double res = std::sqrt((sum_diff * sum_diff - sum_diff2)) / s;
			return std::pair<double, double>(res, res);
		}
		else
		{
			double sum_diff2 = 0;
			double sum_diff = 0;
			double b_sum_diff2 = 0;
			double b_sum_diff = 0;
			int s = height * width;
			int b_sum = 0, sum = 0;
			for (int i = 0; i < s; ++i)
			{
				int diff = std::abs(img[i] - prev[i]);
				if (img_DL[i] > *back)
				{
					sum_diff2 += diff * diff;
					sum_diff += diff;
					sum++;
				}
				else
				{
					b_sum_diff2 += diff * diff;
					b_sum_diff += diff;
					b_sum++;
				}
			}
			return std::pair<double, double>(std::sqrt((b_sum_diff * b_sum_diff - b_sum_diff2)) / b_sum,
											 std::sqrt((sum_diff * sum_diff - sum_diff2)) / sum);
		}
	}

	bool H264_Saver::addImageLossy(const unsigned short *img, int64_t timestamp_ns, const std::map<std::string, std::string> &attributes)
	{
		if (m_data->inputCamera)
			// In this case the input image must be in DL
			return addImageLossyWithCamera(img, timestamp_ns, attributes);
		else
			// In this case the input image must be in T
			return addImageLossyNoCamera(img, timestamp_ns, attributes);
	}

	bool H264_Saver::addImageLossyWithCamera(const unsigned short *img_DL, int64_t timestamp_ns, const std::map<std::string, std::string> &attributes)
	{
		int threads = m_data->threads;
		if (threads < 1)
			threads = 1;

		IRVideoLoader *l = (IRVideoLoader *)get_void_ptr(m_data->inputCamera);
		if (!l)
			return false;

		if (m_data->bp_enabled)
		{
			if (m_data->attributes.size() == 0)
				m_data->bp.init(img_DL, m_data->width, m_data->stop_lossy_height);
			m_data->bp.correct(img_DL, m_data->tmp.data());
			// copy the rest
			std::copy(img_DL + m_data->width * m_data->stop_lossy_height, img_DL + m_data->width * m_data->height, m_data->tmp.data() + m_data->width * m_data->stop_lossy_height);
		}
		else
		{
			// copy the full image
			std::copy(img_DL, img_DL + m_data->width * m_data->height, m_data->tmp.begin());
		}
		{
			// compute integration time
			int s = m_data->width * m_data->stop_lossy_height;
			for (int i = 0; i < s; ++i)
				m_data->IT[i] = m_data->tmp[i] >> 13;
			int s2 = m_data->width * m_data->height;
			for (int i = s; i < s2; ++i)
				m_data->IT[i] = 0;
		}

		if (m_data->attributes.size() == 0)
		{
			// First image

			memcpy(m_data->lastDL.data(), m_data->tmp.data(), m_data->width * m_data->height * 2);

			// convert to T
			l->calibrateInplace(m_data->tmp.data(), m_data->width * m_data->stop_lossy_height, 1);

			if (m_data->subtractMin || m_data->subtractLocalMin)
			{
				// compute min
				m_data->min = 65535;
				int s = m_data->width * m_data->stop_lossy_height;
				for (int i = 0; i < s; ++i)
					if (m_data->tmp[i] < m_data->min)
						m_data->min = m_data->tmp[i];
						// subtract min
#pragma omp parallel for num_threads(threads)
				for (int i = 0; i < s; ++i)
				{
					if (m_data->tmp[i] < m_data->min)
						m_data->tmp[i] = 0;
					else
						m_data->tmp[i] -= m_data->min;
				}

				if (m_data->subtractLocalMin)
					m_data->localMins.push_back(m_data->min);

				addGlobalAttribute("MIN_T", toString(m_data->min));
				addGlobalAttribute("MIN_T_HEIGHT", toString(m_data->stop_lossy_height));
			}

			addGlobalAttribute("STORE_IT", "1");
			addGlobalAttribute("GlobalBackgroundError", toString(m_data->lowValueError));
			addGlobalAttribute("GlobalForegroundError", toString(m_data->highValueError));

			bool res = addImageLossLess(m_data->tmp.data(), m_data->IT.data(), timestamp_ns, attributes);

			m_data->lowError.push_back(m_data->lowValueError);
			m_data->highError.push_back(m_data->highValueError);

			memcpy(m_data->refT.data(), m_data->tmp.data(), m_data->width * m_data->stop_lossy_height * 2);
			memcpy(m_data->prevT.data(), m_data->tmp.data(), m_data->width * m_data->stop_lossy_height * 2);
			return res;
		}

		// convert to T
		std::copy(m_data->tmp.begin(), m_data->tmp.end(), m_data->tmpT.begin());
		l->calibrateInplace(m_data->tmpT.data(), m_data->width * m_data->stop_lossy_height, 1);

		// subtract min to image T
		int s = m_data->width * m_data->stop_lossy_height;

		if (m_data->subtractMin || m_data->subtractLocalMin)
		{
			// recompute min for subtractLocalMin
			if (m_data->subtractLocalMin)
			{
				unsigned min = 65535;
				int s = m_data->width * m_data->stop_lossy_height;
				for (int i = 0; i < s; ++i)
					if (m_data->tmpT[i] < m_data->min)
						m_data->min = m_data->tmpT[i];
				if (min < m_data->min)
					m_data->min = min;
				m_data->localMins.push_back(m_data->min);
				// printf("First image min: %i\n", (int)m_data->min);
			}

			// subtract min T
#pragma omp parallel for num_threads(threads)
			for (int i = 0; i < s; ++i)
			{
				if (m_data->tmpT[i] < m_data->min)
					m_data->tmpT[i] = 0;
				else
					m_data->tmpT[i] -= m_data->min;
			}
		}

		bool res = false;
		unsigned background = get_background(m_data->tmp.data(), m_data->hist, m_data->width * m_data->stop_lossy_height);

		int lowError = m_data->lowValueError;
		int highError = m_data->highValueError;
		std::pair<double, double> std;
		int running_average_frames = 40;
		int start_running_average_frames = 1;
		if ((int)m_data->stdDevs.size() < running_average_frames)
			std = stdDev(m_data->prevT.data(), m_data->tmpT.data(), m_data->width, m_data->stop_lossy_height);
		else
			std = stdDev(m_data->prevT.data(), m_data->tmpT.data(), m_data->width, m_data->stop_lossy_height, img_DL, &background);
		if ((int)m_data->firstStdDevs.size() < start_running_average_frames)
			m_data->firstStdDevs.push_back(std);

		if ((int)m_data->stdDevs.size() < running_average_frames)
		{
			m_data->stdDevs.push_back(std);
		}
		else
		{
			memcpy(m_data->stdDevs.data(), m_data->stdDevs.data() + 1, sizeof(std::pair<double, double>) * (running_average_frames - 1));
			m_data->stdDevs.back() = std;
		}

		// compute mean std
		std::pair<double, double> meanStdDev = m_data->firstStdDevs[0];
		for (size_t i = 1; i < m_data->firstStdDevs.size(); ++i)
		{
			meanStdDev.first += m_data->firstStdDevs[i].first;
			meanStdDev.second += m_data->firstStdDevs[i].second;
		}
		for (size_t i = 0; i < m_data->stdDevs.size(); ++i)
		{
			meanStdDev.first += m_data->stdDevs[i].first;
			meanStdDev.second += m_data->stdDevs[i].second;
		}
		meanStdDev.first /= (m_data->stdDevs.size() + m_data->firstStdDevs.size());
		meanStdDev.second /= (m_data->stdDevs.size() + m_data->firstStdDevs.size());

		highError -= (int)std::round(std::abs(std.second - meanStdDev.second) * m_data->stdFactor);
		lowError -= (int)std::round(std::abs(std.first - meanStdDev.first) * m_data->stdFactor);

		if (highError < 0)
			highError = 0;
		if (lowError < highError)
			lowError = highError;
		// printf("%d: error %d %d\n", m_data->encoder->Count(), lowError, highError);

		auto attrs = attributes;
		attrs["BackgroundError"] = toString(lowError);
		attrs["ForegroundError"] = toString(highError);

		m_data->lowError.push_back(lowError);
		m_data->highError.push_back(highError);

		if (m_data->runningAverage > 0)
			m_data->cum.addImage(m_data->tmpT.data(), threads);

		int size = m_data->width * m_data->stop_lossy_height;

#pragma omp parallel for num_threads(threads)
		for (int i = 0; i < size; ++i)
		{
			int diff = std::abs((int)m_data->tmpT[i] - (int)m_data->refT[i]);
			int max_error = m_data->tmp[i] > background ? highError : lowError;
			if (diff <= max_error && (m_data->lastDL[i] >> 13) == (m_data->tmp[i] >> 13))
			{

				m_data->tmpT[i] = m_data->runningAverage > 0 ? m_data->cum.pixel(i) : m_data->refT[i];
			}
			else
			{
				m_data->refT[i] = m_data->tmpT[i];
				if (m_data->runningAverage > 0)
					m_data->cum.resetPixel(i, m_data->tmpT[i]);
			}
		}

		memcpy(m_data->prevT.data(), m_data->tmpT.data(), m_data->width * m_data->stop_lossy_height * 2);
		memcpy(m_data->lastDL.data(), m_data->tmp.data(), m_data->width * m_data->height * 2);

		// copy the remaining DL values (last X lines)
		std::copy(m_data->tmp.data() + m_data->width * m_data->stop_lossy_height, m_data->tmp.data() + m_data->width * m_data->height, m_data->tmpT.data() + m_data->width * m_data->stop_lossy_height);

		res = addImageLossLess(m_data->tmpT.data(), m_data->IT.data(), timestamp_ns, attrs);

		return res;
	}

	bool H264_Saver::addImageLossyNoCamera(const unsigned short *img, int64_t timestamp_ns, const std::map<std::string, std::string> &attributes)
	{
		int threads = m_data->threads;
		if (threads < 1)
			threads = 1;

		if (m_data->bp_enabled)
		{
			if (m_data->attributes.size() == 0)
				m_data->bp.init(img, m_data->width, m_data->stop_lossy_height);
			m_data->bp.correct(img, m_data->tmp.data());
			// copy the rest
			std::copy(img + m_data->width * m_data->stop_lossy_height, img + m_data->width * m_data->height, m_data->tmp.data() + m_data->width * m_data->stop_lossy_height);
		}
		else
		{
			// copy the full image
			std::copy(img, img + m_data->width * m_data->height, m_data->tmp.begin());
		}

		if (m_data->attributes.size() == 0)
		{
			// First image

			memcpy(m_data->lastDL.data(), m_data->tmp.data(), m_data->width * m_data->height * 2);

			if (m_data->subtractMin)
			{
				// compute min
				m_data->min = 65535;
				int s = m_data->width * m_data->stop_lossy_height;
				for (int i = 0; i < s; ++i)
					if (m_data->tmp[i] < m_data->min)
						m_data->min = m_data->tmp[i];
						// subtract min
#pragma omp parallel for num_threads(threads)
				for (int i = 0; i < s; ++i)
				{
					if (m_data->tmp[i] < m_data->min)
						m_data->tmp[i] = 0;
					else
						m_data->tmp[i] -= m_data->min;
				}

				addGlobalAttribute("MIN_T", toString(m_data->min));
				addGlobalAttribute("MIN_T_HEIGHT", toString(m_data->stop_lossy_height));
			}

			addGlobalAttribute("GlobalBackgroundError", toString(m_data->lowValueError));
			addGlobalAttribute("GlobalForegroundError", toString(m_data->highValueError));

			m_data->lowError.push_back(m_data->lowValueError);
			m_data->highError.push_back(m_data->highValueError);

			bool res = addImageLossLess(m_data->tmp.data(), timestamp_ns, attributes);

			memcpy(m_data->refT.data(), m_data->tmp.data(), m_data->width * m_data->stop_lossy_height * 2);
			memcpy(m_data->prevT.data(), m_data->tmp.data(), m_data->width * m_data->stop_lossy_height * 2);
			return res;
		}

		// convert to T
		std::copy(m_data->tmp.begin(), m_data->tmp.end(), m_data->tmpT.begin());

		// subtract min to image T
		int s = m_data->width * m_data->stop_lossy_height;

		if (m_data->subtractMin)
		{
#pragma omp parallel for num_threads(threads)
			for (int i = 0; i < s; ++i)
			{
				if (m_data->tmpT[i] < m_data->min)
					m_data->tmpT[i] = 0;
				else
					m_data->tmpT[i] -= m_data->min;
			}
		}

		bool res = false;
		unsigned background = get_background(m_data->tmp.data(), m_data->hist, m_data->width * m_data->stop_lossy_height);

		int lowError = m_data->lowValueError;
		int highError = m_data->highValueError;
		std::pair<double, double> std;
		int running_average_frames = 40;
		int start_running_average_frames = 1;
		if ((int)m_data->stdDevs.size() < running_average_frames)
			std = stdDev(m_data->prevT.data(), m_data->tmpT.data(), m_data->width, m_data->stop_lossy_height);
		else
			std = stdDev(m_data->prevT.data(), m_data->tmpT.data(), m_data->width, m_data->stop_lossy_height, img, &background);
		if ((int)m_data->firstStdDevs.size() < start_running_average_frames)
			m_data->firstStdDevs.push_back(std);

		if ((int)m_data->stdDevs.size() < running_average_frames)
		{
			m_data->stdDevs.push_back(std);
		}
		else
		{
			memcpy(m_data->stdDevs.data(), m_data->stdDevs.data() + 1, sizeof(std::pair<double, double>) * (running_average_frames - 1));
			m_data->stdDevs.back() = std;
		}

		// compute mean std
		std::pair<double, double> meanStdDev = m_data->firstStdDevs[0];
		for (size_t i = 1; i < m_data->firstStdDevs.size(); ++i)
		{
			meanStdDev.first += m_data->firstStdDevs[i].first;
			meanStdDev.second += m_data->firstStdDevs[i].second;
		}
		for (size_t i = 0; i < m_data->stdDevs.size(); ++i)
		{
			meanStdDev.first += m_data->stdDevs[i].first;
			meanStdDev.second += m_data->stdDevs[i].second;
		}
		meanStdDev.first /= (m_data->stdDevs.size() + m_data->firstStdDevs.size());
		meanStdDev.second /= (m_data->stdDevs.size() + m_data->firstStdDevs.size());

		highError -= (int)std::round(std::abs(std.second - meanStdDev.second) * m_data->stdFactor);
		lowError -= (int)std::round(std::abs(std.first - meanStdDev.first) * m_data->stdFactor);

		if (highError < 0)
			highError = 0;
		if (lowError < highError)
			lowError = highError;

		auto attrs = attributes;
		attrs["BackgroundError"] = toString(lowError);
		attrs["ForegroundError"] = toString(highError);

		m_data->lowError.push_back(lowError);
		m_data->highError.push_back(highError);

		// printf("%d: error %d %d\n", m_data->encoder->Count(), lowError, highError);
		bool is_key = false;
		/*if (!lowError || !highError)
			is_key = true;*/

		if (m_data->runningAverage > 0)
			m_data->cum.addImage(m_data->tmpT.data(), threads);

		int size = m_data->width * m_data->stop_lossy_height;

#pragma omp parallel for num_threads(threads)
		for (int i = 0; i < size; ++i)
		{
			int diff = std::abs((int)m_data->tmpT[i] - (int)m_data->refT[i]);
			int max_error = m_data->tmp[i] > background ? highError : lowError;
			if (diff <= max_error && (m_data->lastDL[i] >> 13) == (m_data->tmp[i] >> 13))
			{

				m_data->tmpT[i] = m_data->runningAverage > 0 ? m_data->cum.pixel(i) : m_data->refT[i];
			}
			else
			{
				m_data->refT[i] = m_data->tmpT[i];
				if (m_data->runningAverage > 0)
					m_data->cum.resetPixel(i, m_data->tmpT[i]);
			}
		}

		memcpy(m_data->prevT.data(), m_data->tmpT.data(), m_data->width * m_data->stop_lossy_height * 2);
		memcpy(m_data->lastDL.data(), m_data->tmp.data(), m_data->width * m_data->height * 2);

		// copy the remaining DL values (last X lines)
		std::copy(m_data->tmp.data() + m_data->width * m_data->stop_lossy_height, m_data->tmp.data() + m_data->width * m_data->height, m_data->tmpT.data() + m_data->width * m_data->stop_lossy_height);

		res = addImageLossLess(m_data->tmpT.data(), timestamp_ns, attrs, is_key);

		return res;
	}

	bool H264_Saver::addLoss(unsigned short *img)
	{
		int threads = m_data->threads;
		if (threads < 1)
			threads = 1;

		if (m_data->bp_enabled)
		{
			if (m_data->attributes.size() == 0)
				m_data->bp.init(img, m_data->width, m_data->stop_lossy_height);
			m_data->bp.correct(img, m_data->tmp.data());
			// copy the rest
			std::copy(img + m_data->width * m_data->stop_lossy_height, img + m_data->width * m_data->height, m_data->tmp.data() + m_data->width * m_data->stop_lossy_height);
		}
		else
		{
			// copy the full image
			std::copy(img, img + m_data->width * m_data->height, m_data->tmp.begin());
		}

		if (m_data->attributes.size() == 0)
		{
			// First image

			memcpy(m_data->lastDL.data(), m_data->tmp.data(), m_data->width * m_data->height * 2);

			if (m_data->subtractMin)
			{
				// compute min
				m_data->min = 65535;
				int s = m_data->width * m_data->stop_lossy_height;
				for (int i = 0; i < s; ++i)
					if (m_data->tmp[i] < m_data->min)
						m_data->min = m_data->tmp[i];
						// subtract min
#pragma omp parallel for num_threads(threads)
				for (int i = 0; i < s; ++i)
				{
					if (m_data->tmp[i] < m_data->min)
						m_data->tmp[i] = 0;
					else
						m_data->tmp[i] -= m_data->min;
				}

				addGlobalAttribute("MIN_T", toString(m_data->min));
				addGlobalAttribute("MIN_T_HEIGHT", toString(m_data->stop_lossy_height));
			}

			// bool res = addImageLossLess(m_data->tmp.data(), timestamp_ns, attributes);
			++m_data->frameCount;
			m_data->attributes.resize(m_data->attributes.size() + 1);
			m_data->attributes.setTimestamp(m_data->attributes.size() - 1, m_data->frameCount - 1);
			m_data->attributes.setAttributes(m_data->attributes.size() - 1, dict_type());

			memcpy(m_data->refT.data(), m_data->tmp.data(), m_data->width * m_data->stop_lossy_height * 2);
			memcpy(m_data->prevT.data(), m_data->tmp.data(), m_data->width * m_data->stop_lossy_height * 2);

			memcpy(img, m_data->tmp.data(), m_data->width * m_data->stop_lossy_height * 2);

			m_data->lowError.push_back(m_data->lowValueError);
			m_data->highError.push_back(m_data->highValueError);

			return true;
		}

		// convert to T
		std::copy(m_data->tmp.begin(), m_data->tmp.end(), m_data->tmpT.begin());

		// subtract min to image T
		int s = m_data->width * m_data->stop_lossy_height;

		if (m_data->subtractMin)
		{
#pragma omp parallel for num_threads(threads)
			for (int i = 0; i < s; ++i)
			{
				if (m_data->tmpT[i] < m_data->min)
					m_data->tmpT[i] = 0;
				else
					m_data->tmpT[i] -= m_data->min;
			}
		}

		bool res = false;
		unsigned background = get_background(m_data->tmp.data(), m_data->hist, m_data->width * m_data->stop_lossy_height);

		int lowError = m_data->lowValueError;
		int highError = m_data->highValueError;
		std::pair<double, double> std;
		int running_average_frames = 40;
		int start_running_average_frames = 1;
		if ((int)m_data->stdDevs.size() < running_average_frames)
			std = stdDev(m_data->prevT.data(), m_data->tmpT.data(), m_data->width, m_data->stop_lossy_height);
		else
			std = stdDev(m_data->prevT.data(), m_data->tmpT.data(), m_data->width, m_data->stop_lossy_height, img, &background);
		if ((int)m_data->firstStdDevs.size() < start_running_average_frames)
			m_data->firstStdDevs.push_back(std);

		if ((int)m_data->stdDevs.size() < running_average_frames)
		{
			m_data->stdDevs.push_back(std);
		}
		else
		{
			memcpy(m_data->stdDevs.data(), m_data->stdDevs.data() + 1, sizeof(std::pair<double, double>) * (running_average_frames - 1));
			m_data->stdDevs.back() = std;
		}

		// compute mean std
		std::pair<double, double> meanStdDev = m_data->firstStdDevs[0];
		for (size_t i = 1; i < m_data->firstStdDevs.size(); ++i)
		{
			meanStdDev.first += m_data->firstStdDevs[i].first;
			meanStdDev.second += m_data->firstStdDevs[i].second;
		}
		for (size_t i = 0; i < m_data->stdDevs.size(); ++i)
		{
			meanStdDev.first += m_data->stdDevs[i].first;
			meanStdDev.second += m_data->stdDevs[i].second;
		}
		meanStdDev.first /= (m_data->stdDevs.size() + m_data->firstStdDevs.size());
		meanStdDev.second /= (m_data->stdDevs.size() + m_data->firstStdDevs.size());

		double diff_high = std.second < meanStdDev.second ? 0 : std.second - meanStdDev.second;
		double diff_low = std.first < meanStdDev.first ? 0 : std.first - meanStdDev.first;

		highError -= (int)std::round(diff_high * m_data->stdFactor);
		lowError -= (int)std::round(diff_low * m_data->stdFactor);

		if (highError < 0)
			highError = 0;
		if (lowError < highError)
			lowError = highError;

		m_data->lowError.push_back(lowError);
		m_data->highError.push_back(highError);

		// printf("%d: error %d %d\n", m_data->encoder->Count(), lowError, highError);
		bool is_key = false;
		/*if (!lowError || !highError)
			is_key = true;*/

		if (m_data->runningAverage > 0)
			m_data->cum.addImage(m_data->tmpT.data(), threads);

		int size = m_data->width * m_data->stop_lossy_height;

		// printf("%i\n",(int)m_data->runningAverage);fflush(stdout);
#pragma omp parallel for num_threads(threads)
		for (int i = 0; i < size; ++i)
		{
			int diff = std::abs((int)m_data->tmpT[i] - (int)m_data->refT[i]);
			int max_error = m_data->tmp[i] > background ? highError : lowError;
			if (diff <= max_error)
			{

				m_data->tmpT[i] = m_data->runningAverage > 0 ? m_data->cum.pixel(i) : m_data->refT[i];
			}
			else
			{
				m_data->refT[i] = m_data->tmpT[i];
				if (m_data->runningAverage > 0)
					m_data->cum.resetPixel(i, m_data->tmpT[i]);
			}
		}

		memcpy(m_data->prevT.data(), m_data->tmpT.data(), m_data->width * m_data->stop_lossy_height * 2);
		memcpy(m_data->lastDL.data(), m_data->tmp.data(), m_data->width * m_data->height * 2);

		// copy the remaining DL values (last X lines)
		std::copy(m_data->tmp.data() + m_data->width * m_data->stop_lossy_height, m_data->tmp.data() + m_data->width * m_data->height, m_data->tmpT.data() + m_data->width * m_data->stop_lossy_height);

		// res = addImageLossLess(m_data->tmpT.data(), timestamp_ns, attributes, is_key);
		// return res;
		++m_data->frameCount;
		m_data->attributes.resize(m_data->attributes.size() + 1);
		m_data->attributes.setTimestamp(m_data->attributes.size() - 1, m_data->frameCount - 1);
		m_data->attributes.setAttributes(m_data->attributes.size() - 1, dict_type());

		memcpy(img, m_data->tmpT.data(), m_data->width * m_data->stop_lossy_height * 2);
		return true;
	}

	const std::vector<unsigned short> &H264_Saver::lowErrors() const
	{
		return m_data->lowError;
	}
	const std::vector<unsigned short> &H264_Saver::highErrors() const
	{
		return m_data->highError;
	}

#define H264_READ_THREADS 1 // read thread count, must be >= 1

	class VideoGrabber
	{
	public:
		VideoGrabber();
		VideoGrabber(const std::string &name, void *file_access = NULL, int thread_count = H264_READ_THREADS);
		~VideoGrabber();
		bool Open(const std::string &name, void *file_access = NULL, int thread_count = H264_READ_THREADS);
		void Close();

		int ComputeImageCount();

		// Ffmpeg main struct
		AVFormatContext *GetContext() const { return pFormatCtx; }
		// renvoie l'image actuelle
		const std::vector<unsigned short> &GetCurrentFrame();
		const std::vector<unsigned char> &GetCurrentIT();
		const std::vector<unsigned char> &GetITByNumber(int num);

		// position actuelle (en frame), peut etre approximatif
		long int GetCurrentFramePos() const;
		int GetFrameCount() const { return m_frame_count; }
		bool IsLastKeyFrame() const { return m_last_key; }
		// largeur
		int GetWidth() const;
		// hauteur
		int GetHeight() const;
		// frame per second
		double GetFps() const;
		// eventuel offset si le film a ete enregistre avec wolff
		double GetOffset() const { return m_offset; }
		// nom du fichier ouvert
		std::string GetFileName() const;
		;
		bool IsOpen() const;

		// NEW methods
		bool Init();
		const std::vector<unsigned short> &GetFrame(int num);

	public:
		double getTime();
		void toArray(AVFrame *frame);
		void free_packet();

		std::vector<unsigned short> m_image;
		std::vector<unsigned char> m_IT;
		double m_current_time;
		std::string m_filename;
		int m_width;
		int m_height;
		double m_fps;
		double m_tech;
		int m_frame_pos;
		int m_frame_count;
		double m_time_pos;
		double m_offset;
		double m_total_time;
		bool m_file_open;
		bool m_is_packet;
		bool m_last_key;
		int m_thread_count;
		int m_GOP;
		int m_skip_packets;

		// variables ffmpeg
		AVFormatContext *pFormatCtx;
		int videoStream;
		AVCodecContext *pCodecCtx;
		const AVCodec *pCodec;
		AVFrame *pFrame;
		uint8_t *buffer;
		int numBytes;
		AVPacket packet;
		int frameFinished;
	};

	/*static void __destruct(AVPacket * pkt) {
		pkt->data = NULL;
		pkt->size = 0;
	}*/
	static void __init_packet(AVPacket *pkt)
	{
		pkt->pts = 0;
		pkt->dts = 0;
		pkt->pos = -1;
		pkt->duration = 0;
		pkt->flags = 0;
		pkt->stream_index = 0;
	}

	void VideoGrabber::free_packet()
	{
		if (packet.data != NULL && packet.size > 0)
			// if(strcmp((const char*)packet.data,"")!=0)
			av_free(packet.data);

		packet.size = 0;
		packet.data = 0;
		__init_packet(&packet);
	}

	VideoGrabber::VideoGrabber()
	{
		m_file_open = false;
		m_is_packet = false;
		m_last_key = false;
		pFormatCtx = NULL;
		pCodecCtx = NULL;
		pCodec = NULL;
		pFrame = NULL;
		buffer = NULL;
		m_GOP = -1;
		m_thread_count = H264_READ_THREADS;
	}
	VideoGrabber::VideoGrabber(const std::string &name, void *file_reader, int thread_count)
	{
		pFormatCtx = NULL;
		pCodecCtx = NULL;
		pCodec = NULL;
		pFrame = NULL;
		buffer = NULL;
		m_file_open = false;
		m_is_packet = false;
		m_last_key = false;
		m_GOP = -1;
		Open(name, file_reader, thread_count);
	}
	VideoGrabber::~VideoGrabber()
	{
		Close();
	}

	bool VideoGrabber::Open(const std::string &name, void *file_reader, int thread_count)
	{
		{
			m_thread_count = thread_count;
			std::string sthread_count = toString(thread_count);
			m_file_open = true;
			int res = 0;
			unsigned int i = 0;
			__init_packet(&packet);
			packet.data = NULL;
			// Open video file
			if (!name.empty())
			{
#if LIBAVFORMAT_VERSION_MAJOR > 52
				if (avformat_open_input(&pFormatCtx,
										name.c_str(),
										NULL,
										NULL) != 0)
					goto error;
#else
				if (av_open_input_file(&pFormatCtx, name.c_str(), NULL, 0, NULL) != 0)
					goto error;
#endif
			}
			else if (file_reader)
			{
				seekFile(file_reader, 0, AVSEEK_SET);
				unsigned char *buffer = (unsigned char *)av_malloc(4096);
				AVIOContext *pIOCtx = avio_alloc_context(buffer, 4096, // internal Buffer and its size
														 0,			   // bWriteable (1=true,0=false)
														 file_reader,  // user data ; will be passed to our callback functions
														 readFile2,
														 0, // Write callback function (not used in this example)
														 seekFile);
				pFormatCtx = (AVFormatContext *)avformat_alloc_context(); // av_malloc(sizeof(AVFormatContext));
				pFormatCtx->pb = pIOCtx;
#if LIBAVFORMAT_VERSION_MAJOR > 52
				if (avformat_open_input(&pFormatCtx,
										name.c_str(),
										NULL,
										NULL) != 0)
					goto error;
#else
				if (av_open_input_file(&pFormatCtx, name.c_str(), NULL, 0, NULL) != 0)
					goto error;
#endif
			}
			else
			{
				goto error;
			}

			// Retrieve stream information
			if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
				goto error;

			// Find the first video stream
			videoStream = -1;
			for (i = 0; i < pFormatCtx->nb_streams; i++)
#if LIBAVFORMAT_VERSION_MAJOR > 52
				if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
#else
				if (pFormatCtx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO)
#endif
				{
					videoStream = i;
					break;
				}
			if (videoStream == -1)
				goto error;

			// pFormatCtx->cur_st = pFormatCtx->streams[0];

			// Get a pointer to the codec context for the video stream

			// avcodec_find_decoder(pFormatCtx->streams[videoStream]->codecpar->codec_id);

			// Find the decoder for the video stream
			// pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
			// pCodecCtx = avcodec_alloc_context3(pCodec);

			// Get a pointer to the codec context for the video stream
			pCodec = avcodec_find_decoder(pFormatCtx->streams[videoStream]->codecpar->codec_id);
			pCodecCtx = avcodec_alloc_context3(pCodec);
			avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoStream]->codecpar);
			// pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
			//  avcodec_find_decoder(pFormatCtx->streams[videoStream]->codecpar->codec_id);

			// Find the decoder for the video stream
			// pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
			// pCodecCtx = avcodec_alloc_context3(pCodec);

			if (pCodec == NULL)
				goto error;

			// Open codec
			pCodecCtx->thread_count = m_thread_count;
			// pCodecCtx->thread_type = FF_THREAD_SLICE;
			av_opt_set(pCodecCtx->priv_data, "threads", sthread_count.c_str(), AV_OPT_SEARCH_CHILDREN);
			av_opt_set(pCodecCtx, "threads", sthread_count.c_str(), AV_OPT_SEARCH_CHILDREN);
			// pCodecCtx->thread_type = FF_THREAD_SLICE;
			// TODO
			// pCodecCtx->thread_count = 1;

			// set number of thread for reading
			pCodecCtx->thread_count = thread_count; // 4;
			pCodecCtx->thread_type = FF_THREAD_SLICE;

			res = avcodec_open2(pCodecCtx, pCodec, NULL);
			if (res < 0)
				goto error;

			// Allocate video frame
			pFrame = av_frame_alloc();
			if (pFrame == NULL)
				goto error;
			
			// Determine required buffer size and allocate buffer
			//numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, pCodecCtx->width,
			//									pCodecCtx->height, 1);

			//buffer = (uint8_t *)av_malloc(numBytes);

			// Assign appropriate parts of buffer to image planes in pFrameRGB
			//av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize,
			//					 buffer, AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1);

			// Initialize Context
			if (pCodecCtx->pix_fmt == AV_PIX_FMT_NONE)
				pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

			m_width = pCodecCtx->width;
			m_height = pCodecCtx->height;
			if (pFormatCtx->metadata)
			{
				// for kvazaar encoder, the real width and height are stored in the comment section
				AVDictionaryEntry *tag = nullptr;
				if ((tag = av_dict_get(pFormatCtx->metadata, "comment", tag, AV_DICT_IGNORE_SUFFIX)))
				{
					std::string str = tag->value;
					if (str.find("size:") == 0)
					{
						str = str.substr(5);
						replace(str, "x", " ");
						std::istringstream iss(str);
						int w, h;
						if (iss >> w >> h)
						{
							m_width = w;
							m_height = h;
						}
					}
				}
			}

			m_image.resize(m_height * m_width);
			m_IT.resize(m_height * m_width);

			m_fps = pFormatCtx->streams[videoStream]->r_frame_rate.num * 1.0 / pFormatCtx->streams[videoStream]->r_frame_rate.den * 1.0;
			m_frame_count = (int)pFormatCtx->streams[videoStream]->nb_frames;

			m_tech = 1 / m_fps;

			m_frame_pos = 0;
			m_time_pos = 0;

			m_total_time = (double)pFormatCtx->duration / AV_TIME_BASE;
			m_offset = 0;
			m_filename = name;

			if (m_frame_count == 0)
			{
				m_frame_count = (int)(m_total_time * m_fps);
			}
		}

		return Init();

	error:
		Close();
		return false;
	}

	double VideoGrabber::getTime()
	{
		av_seek_frame(pFormatCtx, videoStream, 0, AVSEEK_FLAG_BACKWARD);
		int count = 0;
		while (av_read_frame(pFormatCtx, &packet) == 0)
		{
			if (packet.stream_index == videoStream)
				count++;
		}

		av_seek_frame(pFormatCtx, videoStream, 0, AVSEEK_FLAG_BACKWARD);
		return count / m_fps;
	}

	bool VideoGrabber::IsOpen() const
	{
		return m_file_open;
	}
	void VideoGrabber::Close()
	{
		if (m_file_open)
		{
			// Free the YUV frame
			if (pFrame != NULL)
			{
				av_frame_free(&pFrame);
			}

			// Close the codec
			if (pCodecCtx != NULL && pCodec != NULL)
			{
				avcodec_close(pCodecCtx);
				avcodec_free_context(&pCodecCtx);
			}
			// Close the video file
			if (pFormatCtx != NULL)
				avformat_close_input(&pFormatCtx);

			if (packet.data)
				av_packet_unref(&packet);

			if (buffer)
				av_free(buffer);
		}

		pFormatCtx = NULL;
		pCodecCtx = NULL;
		pCodec = NULL;
		pFrame = NULL;
		buffer = NULL;
		m_file_open = false;
		m_is_packet = false;
	}

	const std::vector<unsigned short> &VideoGrabber::GetCurrentFrame()
	{
		return m_image;
	}

	const std::vector<unsigned char> &VideoGrabber::GetCurrentIT()
	{
		return m_IT;
	}

	void VideoGrabber::toArray(AVFrame *frame)
	{
		unsigned short *data = (unsigned short *)m_image.data();
		unsigned char *IT = (unsigned char *)m_IT.data();

		if (pCodecCtx->pix_fmt == AV_PIX_FMT_YUV444P)
		{
			for (int y = 0; y < frame->height; ++y)
			{
				unsigned char *d0 = frame->data[0] + y * frame->linesize[0];
				unsigned char *d1 = frame->data[1] + y * frame->linesize[1];
				unsigned char *d2 = frame->data[2] + y * frame->linesize[2];
				for (int i = 0; i < frame->width; ++i)
				{
					data[i + y * frame->width] = d1[i] | (d2[i] << 8);
					IT[i + y * frame->width] = d0[i];
				}
			}
		}
		else if (pCodecCtx->pix_fmt == AV_PIX_FMT_YUV420P)
		{
			for (int y = 0; y < m_height; ++y)
			{
				unsigned char *d0 = frame->data[0] + y * frame->linesize[0];
				unsigned char *d0_2 = frame->data[0] + (y + m_height) * frame->linesize[0];
				unsigned char *d1 = frame->data[1] + y * frame->linesize[1];
				for (int i = 0; i < m_width; ++i)
				{
					data[i + y * m_width] = d0[i] | (d0_2[i] << 8);
					IT[i + y * frame->width] = d1[i];
				}
			}
		}

		m_last_key = frame->key_frame || (frame->pict_type == AV_PICTURE_TYPE_I);
	}

	bool VideoGrabber::Init()
	{
		{

			m_skip_packets = 0;
			AVPacket p;
			av_init_packet(&p);

			int finish = 0;
			while (finish == 0)
			{
				if (p.buf)
				{
					av_packet_unref(&p);
				}
				if (av_read_frame(pFormatCtx, &p) < 0)
				{
					av_init_packet(&p);
				}

				int ret = decode(pCodecCtx, pFrame, &finish, &p);
				if (ret <= 0 && pFrame->data[0] == NULL)
				{
					if (p.buf)
					{
						av_packet_unref(&p);
					}
					return false;
				}
				else if (finish)
					break;
				++m_skip_packets;
			}
			if (p.buf)
			{
				av_packet_unref(&p);
			}
			toArray(pFrame);
			m_frame_pos = 0;
			// int value = m_image[0];
		}
		return true;
	}
	const std::vector<unsigned short> &VideoGrabber::GetFrame(int num)
	{
		static const std::vector<unsigned short> null_image;

		if (num == m_frame_pos)
			return m_image;

		AVPacket p;
		av_init_packet(&p);

		// if dist < 16 frame, keep reading frames
		int max_dist = std::max(16, m_skip_packets * 2);
		if (m_frame_pos >= 0 && num >= m_frame_pos && num - m_frame_pos <= max_dist)
		{

			int diff = (num - m_frame_pos);
			for (int i = 0; i < diff; ++i)
			{
				if (p.buf)
					av_packet_unref(&p);
				if (av_read_frame(pFormatCtx, &p) < 0)
					av_init_packet(&p);
				int finish = 0;
				int ret = decode(pCodecCtx, pFrame, &finish, &p);
				if (finish == 0 || (ret <= 0 && pFrame->data[0] == NULL))
				{
					if (p.buf)
						av_packet_unref(&p);
					m_frame_pos = -1; // in case of error, invalidate m_frame_pos to be sure to call av_seek_frame next time
					return null_image;
				}
			}
			toArray(pFrame);
			m_frame_pos = num;
			if (p.buf)
				av_packet_unref(&p);
			int value = m_image[0];
			return m_image;
		}

		if (m_skip_packets && num < m_skip_packets)
		{
			// first frames: restart from the first one. This is mandatory for movies with a size of m_skip_packets.
			int ret = av_seek_frame(pFormatCtx, videoStream, 0, AVSEEK_FLAG_BACKWARD);
			avcodec_flush_buffers(pCodecCtx);

			int count = 0;
			while (true)
			{
				if (p.buf)
					av_packet_unref(&p);
				if (av_read_frame(pFormatCtx, &p) < 0)
					av_init_packet(&p);
				int finish = 0;
				int ret = decode(pCodecCtx, pFrame, &finish, &p);
				if (ret <= 0 && pFrame->data[0] == NULL)
				{
					if (p.buf)
						av_packet_unref(&p);
					m_frame_pos = -1; // in case of error, invalidate m_frame_pos to be sure to call av_seek_frame next time
					return null_image;
				}
				if (finish)
				{
					if (count == num)
					{
						m_frame_pos = num;
						toArray(pFrame);
						if (p.buf)
							av_packet_unref(&p);
						int value = m_image[0];
						return m_image;
					}
					++count;
				}
			}
		}
		else if (m_skip_packets && m_frame_count - m_skip_packets * 2 < num)
		{
			// for the last frames (and only if m_skip_packets is > 0), we first seek to m_frame_count - m_skip_packets *2,
			// and then we go to the requested frame
			int pos = m_frame_count - m_skip_packets * 2;
			if (pos < 0)
				pos = 0;
			GetFrame(pos);
			return GetFrame(num);
		}

		int ret = av_seek_frame(pFormatCtx, videoStream, (num) * 12800, AVSEEK_FLAG_BACKWARD);

		avcodec_flush_buffers(pCodecCtx);
		if (ret < 0)
			return null_image;

		int64_t target_dts = (num + m_skip_packets) * 12800;

		while (true)
		{
			int finish = 0;
			while (finish == 0)
			{
				if (p.buf)
				{
					av_packet_unref(&p);
				}
				if (av_read_frame(pFormatCtx, &p) < 0)
				{
					av_init_packet(&p);
				}

				int ret = decode(pCodecCtx, pFrame, &finish, &p);
				if (ret <= 0 && pFrame->data[0] == NULL)
				{
					if (p.buf)
					{
						av_packet_unref(&p);
					}
					m_frame_pos = -1; // in case of error, invalidate m_frame_pos to be sure to call av_seek_frame next time
					return null_image;
				}
			}

			if (pFrame->pkt_dts == target_dts)
			{
				break;
			}
		}
		toArray(pFrame);
		m_frame_pos = num;
		if (p.buf)
			av_packet_unref(&p);
		int value = m_image[0];
		return m_image;
	}

	int VideoGrabber::ComputeImageCount()
	{
		// int ret = av_seek_frame(pFormatCtx, videoStream, 150*12800,  AVSEEK_FLAG_BACKWARD);

		AVPacket p;
		av_init_packet(&p);
		this;
		int count = 0;
		while (true)
		{

			int finish = 0;
			while (finish == 0)
			{
				if (p.buf)
				{
					av_packet_unref(&p);
					// av_init_packet(&p);
				}
				if (av_read_frame(pFormatCtx, &p) < 0)
				{
					av_init_packet(&p);
				}

				int ret = decode(pCodecCtx,
								 pFrame,
								 &finish,
								 &p);
				if (ret < 0)
					bool stop = true;
			}
			++count;
			toArray(pFrame);
			size_t dts = pFrame->pkt_dts; // USE pkt_dts
			int value = m_image[0];
			bool stop = true;
		}
		return count;
	}

	const std::vector<unsigned char> &VideoGrabber::GetITByNumber(int number)
	{
		GetFrame(number);
		return GetCurrentIT();
	}

	int VideoGrabber::GetWidth() const { return m_width; }
	int VideoGrabber::GetHeight() const { return m_height; }
	std::string VideoGrabber::GetFileName() const { return m_filename; }
	long int VideoGrabber::GetCurrentFramePos() const { return m_frame_pos; }
	double VideoGrabber::GetFps() const { return m_fps; }

	struct ReadThread
	{
		struct Img
		{
			std::vector<unsigned short> data;
			int pos;
		};

		H264_Loader *loader;
		int frameStep;
		int frameFinished;
		int frameRequest;
		int frameLastRequest;
		Img null;
		Img res;
		std::map<int, Img> imgs;
		std::mutex mutex;
		std::thread *th;

		ReadThread(H264_Loader *l)
			: loader(l), frameStep(0), frameFinished(0), frameRequest(-1), frameLastRequest(-1)
		{
			null.pos = res.pos = -1;
			th = new std::thread(std::bind(&ReadThread::start, this));
		}
		~ReadThread()
		{
			loader = NULL;
			th->join();
			delete th;
		}

		Img pop(int pos)
		{
			std::lock_guard<std::mutex> lock(mutex);
			std::map<int, Img>::const_iterator it = imgs.find(pos);
			if (it != imgs.end())
			{
				Img res = std::move(it->second);
				imgs.erase(it);
				return res;
			}
			return null;
		}

		void push(const Img &img)
		{
			std::lock_guard<std::mutex> lock(mutex);
			std::map<int, Img>::const_iterator it = imgs.find(img.pos);
			if (it != imgs.end())
				return;
			imgs.insert(std::pair<int, Img>(img.pos, std::move(img)));
		}

		void start()
		{
			while (H264_Loader *l = loader)
			{
				if (frameRequest >= 0)
				{

					frameStep = frameRequest - frameLastRequest;
					frameLastRequest = frameRequest;

					int saved = frameRequest;
					frameRequest = -1;
					// look for img
					res = pop(saved);
					if (res.pos >= 0)
						frameFinished = 1;
					else
					{
						res.pos = saved;
						res.data.resize(l->imageSize().width * l->imageSize().height);
						l->readImageInternal(saved, res.data.data());
						frameFinished = 1;
					}

					// read next
					if (frameStep && std::abs(frameStep) < 8)
					{
						int new_pos = saved + frameStep;
						if (new_pos < 0 || new_pos >= l->size())
							continue;
						std::lock_guard<std::mutex> lock(mutex);
						std::map<int, Img>::const_iterator it = imgs.find(saved + frameStep);
						if (it == imgs.end())
						{
							Img tmp;
							tmp.pos = saved + frameStep;
							tmp.data.resize(l->imageSize().width * l->imageSize().height);
							l->readImageInternal(saved + frameStep, tmp.data.data());
							imgs.insert(std::pair<int, Img>(tmp.pos, std::move(tmp)));
						}
					}
				}
			}
		}

		bool get(int pos, unsigned short *pixels)
		{
			if (pos < 0 || pos >= loader->size())
				return false;
			if (res.pos == pos)
			{
				std::copy(res.data.begin(), res.data.end(), pixels);
				return true;
			}
			frameFinished = 0;
			frameRequest = pos;
			while (frameFinished != 1)
				msleep(1);
			std::copy(res.data.begin(), res.data.end(), pixels);
			return true;
		}
	};

	class H264_Loader::PrivateData
	{
	public:
		VideoGrabber grabber;
		FileAttributes attrs;
		std::vector<int64_t> timestamps;
		int readThreadCount;
		std::vector<std::vector<unsigned short>> firstImages;
		std::vector<std::vector<unsigned short>> lastImages;
		void *file_reader;
		// ReadThread *th;
	};

	H264_Loader::H264_Loader()
	{
		m_data = new PrivateData();
		m_data->readThreadCount = H264_READ_THREADS;
		m_data->file_reader = NULL;
		// m_data->th = new ReadThread(this);
	}
	H264_Loader::~H264_Loader()
	{
		close();
		delete m_data;
	}

	void H264_Loader::setReadThreadCount(int count)
	{
		if (!m_data->grabber.IsOpen())
			m_data->readThreadCount = count;
	}
	int H264_Loader::readThreadCount() const
	{
		return m_data->readThreadCount;
	}

	bool H264_Loader::isValidFile(const char *filename)
	{
		VideoGrabber g;
		return g.Open(filename);
	}

	bool H264_Loader::open(const char *filename)
	{
		if (!file_exists(filename))
			return false;
		m_data->file_reader = createFileReader(createFileAccess(filename));
		return open(m_data->file_reader);
	}

	const FileAttributes *H264_Loader::fileAttributes() const
	{
		return &m_data->attrs;
	}

	bool H264_Loader::open(void *file_reader)
	{
		int threads = m_data->readThreadCount;
		if (threads <= 0)
			threads = 1;
		// TEST: 2 threads instead of 1
		// open the video file
		if (m_data->grabber.Open(std::string(), file_reader, threads))
		{
			if (m_data->attrs.openReadOnly(file_reader))
			{
				if ((int)m_data->attrs.size() == m_data->grabber.GetFrameCount())
				{
					m_data->timestamps.resize(m_data->attrs.size());
					for (size_t i = 0; i < m_data->timestamps.size(); ++i)
						m_data->timestamps[i] = m_data->attrs.timestamp(i);
				}
				else
				{
					m_data->timestamps.resize(m_data->grabber.GetFrameCount());
					int64_t date = 0;
					for (size_t i = 0; i < m_data->timestamps.size(); ++i)
					{
						m_data->timestamps[i] = date;
						date += 20000000;
					}
				}

				// Read the GOP value, that will help optimizing the frame reading
				std::map<std::string, std::string>::const_iterator it = m_data->attrs.globalAttributes().find("GOP");
				if (it != m_data->attrs.globalAttributes().end())
				{
					bool ok = false;
					int gop = fromString<int>(it->second, &ok);
					if (ok && gop > 0)
						m_data->grabber.m_GOP = gop;
				}

				return true;
			}
			else
			{
				return false;
			}
		}
		return false;
	}

	int H264_Loader::size() const
	{
		return m_data->grabber.GetFrameCount();
	}
	const TimestampVector &H264_Loader::timestamps() const
	{
		return m_data->timestamps;
	}
	Size H264_Loader::imageSize() const
	{
		return Size(m_data->grabber.GetWidth(), m_data->grabber.GetHeight());
	}

	bool H264_Loader::readImageInternal(int pos, unsigned short *pixels)
	{
		if (pos < 0 || pos >= size())
			return false;

		// thread count ==1: standard reading
		m_data->grabber.GetFrame(pos);
		std::copy(m_data->grabber.GetCurrentFrame().begin(), m_data->grabber.GetCurrentFrame().end(), pixels);

		return true;
	}

	bool H264_Loader::readImage(int pos, int, unsigned short *pixels)
	{
		// return m_data->th->get(pos, pixels);
		return readImageInternal(pos, pixels);
	}
	const std::vector<unsigned char> &H264_Loader::lastIt() const
	{
		return m_data->grabber.GetCurrentIT();
	}

	bool H264_Loader::getRawValue(int x, int y, unsigned short *value) const
	{
		if (x < 0)
			return false;
		else if (x >= m_data->grabber.GetWidth())
			return false;
		if (y < 0)
			return false;
		else if (y >= m_data->grabber.GetHeight())
			return false;

		*value = m_data->grabber.GetCurrentFrame()[x + y * m_data->grabber.GetWidth()];
		return true;
	}

	const std::map<std::string, std::string> &H264_Loader::globalAttributes() const
	{
		return m_data->attrs.globalAttributes();
	}

	bool H264_Loader::isValid() const
	{
		return m_data->grabber.IsOpen();
	}

	void H264_Loader::close()
	{
		m_data->grabber.Close();
		m_data->attrs.close();
		if (m_data->file_reader)
		{
			destroyFileReader(m_data->file_reader);
			m_data->file_reader = NULL;
		}
	}

	std::string H264_Loader::filename() const
	{
		return m_data->grabber.GetFileName();
	}

	bool H264_Loader::extractAttributes(std::map<std::string, std::string> &attrs) const
	{
		int pos = m_data->grabber.GetCurrentFramePos();
		if (pos >= (int)m_data->attrs.size())
			attrs.clear();
		else
			attrs = m_data->attrs.attributes(pos);

		return true;
	}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

}
