#pragma once

#include <functional>

#include "IRVideoLoader.h"
#include "FileAttributes.h"

namespace rir
{

	IO_EXPORT void setFFmpegLogEnabled(bool enable);


	class IO_EXPORT VideoDownsampler
	{
	public:
		using callback_type = std::function<void(const unsigned short*, std::int64_t, const std::map<std::string, std::string>&)>;

		VideoDownsampler();
		~VideoDownsampler();

		bool open(int width, int height, int lossy_height, int factor, double factor_std, callback_type callback);
		int close();

		bool addImage(const unsigned short* img, std::int64_t timestamp, const std::map<std::string, std::string>& attributes);
		bool addImage2(const unsigned short* img, std::int64_t timestamp, const std::map<std::string, std::string>& attributes);
	private:
		class PrivateData;
		PrivateData* d_data;
	};

	/// @brief Class implementing lossless/lossy compression of infrared videos as described in article [].
	///
	/// H264_Saver class is used to generate compressed IR video files with additional attributes.
	/// It generates MP4 video files containing IR images on 16 bits per pixel, and any kind of global/frame attributes (see FileAttributes class for more details).
	///
	/// The output video is compressed with either H264 or HEVC video codec (based onf ffmpeg library) using its lossless compression mode. The potential losses are
	/// added by the H264_Saver itself in order to maximize the video compressibility. The optional temperature losses are bounded and cannot be exceeded.
	///
	/// This class should not be used to mix lossy and lossless frames within the same video file.
	///
	class IO_EXPORT H264_Saver
	{
	public:
		H264_Saver();
		~H264_Saver();

		/// @brief Returns true is the saver is open, false otherwise
		bool isOpen() const;

		/// @brief Open saver to record a movie in given file
		/// @param filename output filename
		/// @param width video width
		/// @param height video height
		/// @param stop_lossy_height apply lossy compression only up to given row (remaining rows are compressed lossless). In most cases, stop_lossy_height == height.
		/// @param fps video frame rate. This is only used if you intend to read the video with a standard player like VLC. Otherwise, what matter are the timestamps given in addImageLossLess() or addImageLossy()
		/// @return true on success, false otherwise
		bool open(const char *filename, int width, int height, int stop_lossy_height, int fps);

		/// @brief Close recorded video file and write attributes (if any).
		void close();

		/// @brief Add a global video attribute.
		/// Global video attributes can be added until the call to close().
		/// @param key attribute key, ascii null terminated
		/// @param value attribute value, any binary format
		void addGlobalAttribute(const std::string &key, const std::string &value);

		/// @brief Remove global attributes
		void clearGlobalAttributes();

		/// @brief Set global attributes, discarding any global attribute previously set.
		/// Global video attributes can be added until the call to close().
		void setGlobalAttributes(const std::map<std::string, std::string> &attributes);

		/// @brief Add image to the video saver that will be compressed in a lossless way
		/// @param img input image
		/// @param timestamp_ns image timestamp in nanoseconds
		/// @param attributes image attributes
		/// @param is_key if true, force this frame to become a key frame. If false, the GOP (Group Of Pictures) attribute is used to determine if this is a key frame.
		/// @return true on success, false otherwise
		bool addImageLossLess(const unsigned short *img, int64_t timestamp_ns, const std::map<std::string, std::string> &attributes, bool is_key = false);

		/// @brief Add image to the video saver that will be compressed in a lossless way
		/// @param img input image
		/// @param IT integration time image. Internal video codec compresses 24 bits images (YUV). This is a way to store additional 8 bits information per pixel, usually used to store pixels integration times.
		/// @param timestamp_ns image timestamp in nanoseconds
		/// @param attributes image attributes
		/// @param is_key if true, force this frame to become a key frame. If false, the GOP (Group Of Pictures) attribute is used to determine if this is a key frame.
		/// @return true on success, false otherwise
		bool addImageLossLess(const unsigned short *img, const unsigned char *IT, int64_t timestamp_ns, const std::map<std::string, std::string> &attributes, bool is_key = false);

		/// @brief Add image to the video saver that will be compressed in a lossy way.
		/// The losses are separatly defined for the image background (low temperature values) and foreground (high temlperature values).
		/// The separation value between background and foreground is computed based on the image histogram. You can set the same error for both low and high temperature values.
		/// The maximum allowed temperature errors are set with setLowValueError() and setHighValueError().
		/// @param img_DL_or_T if the "inputCamera" parameter is not set (or set to 0), the input image should be in temperature directly.
		/// If "inputCamera" is set to a valid video loader identifier (see set_void_ptr() and get_void_ptr() functions), the input image should be stored in DL instead.
		/// In this case, the input camera calibration is used to calibrate input DL image. The 3 most significant bits of the input DL image are considered to be integration times
		/// and are saved in a lossless way.
		/// @param timestamp_ns image timestamp in nanoseconds
		/// @param attributes image attributes
		/// @return true on success, false otherwise.
		bool addImageLossy(const unsigned short *img_DL_or_T, int64_t timestamp_ns, const std::map<std::string, std::string> &attributes);

		/// @brief Set the compression level, from 0 (default) to 8 (maximum compression).
		/// For h264 video codec, levels correspond to the presets: ultrafast, superfast, veryfast, faster, fast, medium, slow, slower, veryslow.
		/// @param clevel compression level
		void setCompressionLevel(int clevel);
		int compressionLevel() const;

		/// @brief Set the maximum temperature error for low temperature values ( < background value).
		/// @param max_error_T maximum error
		void setLowValueError(int max_error_T);
		int lowValueError() const;

		/// @brief Set the maximum temperature error for high temperature values ( > background value).
		/// @param max_error_T maximum error
		void setHighValueError(int max_error_T);
		int highValueError() const;

		/// @brief Generic way to set parameters passed as string values.
		/// Parameters must be all set before the call to open().
		/// Currently supported parameters are:
		///		-	lowValueError: see setLowValueError().
		///		-	highValueError: see setHighValueError().
		///		-	compressionLevel: see setCompressionLevel().
		///		-	codec: video codec, either "h264" (default) or "h265".
		///		-	GOP: Group Of Picture defining the interval between key frames. Default to 50.
		///		-	threads: number of threads used for compression and loss introduction algorithm. Default to 1.
		///		-	slices: number of slices used by the video codec. This allows multithreaded decompression at the frame level. Default to 1.
		///		-	stdFactor: factor used by the error reduction mechanism as described in []. Default to 5.
		///		-	inputCamera: input camera identifier used for the DL to temperature calibration (see addImageLossy() function). Default to 0 (disabled).
		///		-	removeBadPixels: remove images bad pixels if set to 1. Default to 0 (disabled)
		///		-	runningAverage: running average length as described in []. Default to 32.
		///
		/// @param key parameter name
		/// @param value parameter value
		/// @return true on success, false otherwise.
		bool setParameter(const char *key, const char *value);
		std::string getParameter(const char *key) const;

		/// @brief Apply the loss introduction algorithm to input temperature image.
		/// If you intend to use this function, you should not call the other compression functions (addImageLossLess() and addImageLossy()).
		/// More generally, a H264_Saver should be used for lossless compression, lossy compression, or adding losses, but without merging these 3 options.
		/// A H264_Saver object calling this function (only) do not need to be open on a valid output file.
		/// @param img_T input image
		/// @return true on success, false otherwise.
		bool addLoss(unsigned short *img_T);

		/// @brief Returns the low error value for each compressed frame so far.
		const std::vector<unsigned short> &lowErrors() const;
		/// @brief Returns the high error value for each compressed frame so far.
		const std::vector<unsigned short> &highErrors() const;

	private:
		bool addImageLossyWithCamera(const unsigned short *img_DL, int64_t timestamp_ns, const std::map<std::string, std::string> &attributes);
		bool addImageLossyNoCamera(const unsigned short *img_T, int64_t timestamp_ns, const std::map<std::string, std::string> &attributes);

		class PrivateData;
		PrivateData *m_data;
	};

	/// @brief IR video loader used to read back videos recorded by H264_Saver class
	///
	class IO_EXPORT H264_Loader : public IRVideoLoader
	{
	public:
		H264_Loader();
		~H264_Loader();

		/// @brief Check if given file can be open by this class
		static bool isValidFile(const char *filename);

		/// @brief Open given file
		/// @param filename input filename
		/// @return true on success, false otherwise
		bool open(const char *filename);

		/// @brief Open given file using a file reader object created with createFileReader()
		/// @param file_reader file reader object
		/// @return true on success, false otherwise
		bool open(const FileReaderPtr &file_reader);

		/// @brief Set the number of threads used to decompress each frame.
		/// This number is bounded by the "slices" parameter set through H264_Saver::setParameter().
		/// This function must be called BEFORE opening the video with H264_Loader::open().
		/// @param thread number, default to 1
		void setReadThreadCount(int);
		int readThreadCount() const;

		/// @brief Reimplemented from IRVideoLoader
		virtual bool supportBadPixels() const { return false; }
		/// @brief Returns the total number of frames within the video
		virtual int size() const;
		/// @brief Returns all images timestamps in nanoseconds
		virtual const TimestampVector &timestamps() const;
		/// @brief Returns the image size
		virtual Size imageSize() const;
		/// @brief Read the image at given position
		/// @param pos image position
		/// @param calibration ignored
		/// @param pixels output image
		/// @return true on success, false otherwise
		virtual bool readImage(int pos, int calibration, unsigned short *pixels);

		/// @brief Returns the integration time image for the last read image.
		const std::vector<unsigned char> &lastIt() const;

		/// @brief Reimplemented from IRVideoLoader
		virtual CalibrationPtr calibration() const { return CalibrationPtr(); }
		virtual bool setCalibration(const CalibrationPtr&) { return false; }
		/// @brief Reimplemented from IRVideoLoader
		virtual bool getRawValue(int x, int y, unsigned short *value) const;
		/// @brief Reimplemented from IRVideoLoader
		virtual const std::map<std::string, std::string> &globalAttributes() const;
		/// @brief Reimplemented from IRVideoLoader
		virtual const char *typeName() const
		{
			return "H264_Loader";
		}
		/// @brief Reimplemented from IRVideoLoader
		virtual StringList supportedCalibration() const
		{
			return StringList();
		};
		/// @brief Returns true is the video file was successfully open.
		virtual bool isValid() const;
		/// @brief Reimplemented from IRVideoLoader
		virtual void close();
		/// @brief Reimplemented from IRVideoLoader
		virtual std::string filename() const;
		/// @brief Reimplemented from IRVideoLoader
		virtual bool extractAttributes(std::map<std::string, std::string> &) const;

		const FileAttributes* fileAttributes() const;

		bool readImageInternal(int pos, unsigned short *pixels);

	private:
		class PrivateData;
		PrivateData *m_data;
	};

} // end rir
