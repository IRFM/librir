
#include "BaseCalibration.h"
#include "LutCalibration.h"
#include <memory>

namespace rir
{
    class LutCalibration::PrivateData
    {

    public:
        std::string error;
        std::string warning;
        std::string camera;
        std::string lut_filename;
        Array2D<float> lut;
    };

    int LutCalibration::parse_lut_filename(const std::string &lut_filename)
    {
        m_data->lut = readFileFast<float>(lut_filename.c_str(), &m_data->error);
        // m_data->lut.resize(3 * 2, 0.f);
        m_data->warning += m_data->error.empty() ? std::string() : ((m_data->warning.empty() ? "" : "\n") + m_data->error);
        if (!isValid())
            return 1;
        float max_DL = m_data->lut.values[(m_data->lut.height * m_data->lut.width) - 2]; // c'est la max DL. à trier avec un std::max like
        m_DL_to_temp.resize(static_cast<size_t>(max_DL) + 1);
        size_t last_dl = 0;
        size_t last_temp = 0;

        for (size_t i = 0; i < m_data->lut.height; ++i)
        {
            float DL = m_data->lut.values[i * 2];
            float temp = m_data->lut.values[i * 2 + 1];
            m_DL_to_temp[DL] = temp;
            for (size_t j = last_dl; j < DL; j++)
            {
                m_DL_to_temp[j] = last_temp;
            }
            last_dl = DL + 1;
            last_temp = temp;
        }
        // std::vector<float> &m_DL_to_temp = 0;
        int index = 0;
        float temp = (int)m_DL_to_temp[0];
        for (size_t i = 0; i < m_DL_to_temp.size(); ++i)
        {
            if (m_DL_to_temp[i] != temp)
            {
                // interpol values between index and i-1
                int count = (int)i - index;
                if (count > 1)
                {
                    float lower = (float)temp;
                    float upper = m_DL_to_temp[i];
                    float step = (upper - lower) / (float)count;
                    int c = 0;
                    for (int j = index; j < (int)i; ++j, ++c)
                        m_DL_to_temp[j] = lower + c * step;
                }

                index = (int)i;
                temp = (int)m_DL_to_temp[i];
            }
        }
        // last values
        size_t count = m_DL_to_temp.size() - index;
        if (count > 1)
        {
            float lower = (float)temp;
            float upper = m_DL_to_temp.back();
            float step = (upper - lower) / (float)count;
            for (int j = index; j < (int)m_DL_to_temp.size(); ++j)
                m_DL_to_temp[j] = lower + j * step;
        }

        // m_temp_toDL.resize(m_data->lut.height * m_data->lut.width);
        return 0;
    }

    LutCalibration::LutCalibration(const std::string &lut_filename)
    {
        m_data = new PrivateData();
        m_data->lut_filename = lut_filename;
        parse_lut_filename(lut_filename);
    }
    LutCalibration::LutCalibration() { m_data = new PrivateData(); }
    bool LutCalibration::isValid() const
    {
        return m_data->error.empty();
    }

    void LutCalibration::setLutFilename(std::string lut_filename)
    {
        m_data->lut_filename = lut_filename;
    }
    std::string LutCalibration::error() const { return m_data->error; }
    std::string LutCalibration::warning() const { return m_data->warning; }
    std::string LutCalibration::lutFilename() const { return m_data->lut_filename; }

    unsigned LutCalibration::rawDLToTemp(unsigned DL, int ti = 0) const
    {
        return static_cast<unsigned>(rawDLToTempF(DL, 0));
    }
    float LutCalibration::rawDLToTempF(unsigned DL, int ti = 0) const
    {
        if (m_DL_to_temp.size() == 0)
            return 0;
        if (DL > m_DL_to_temp.size())
            DL = m_DL_to_temp.size() - 1;
        return (m_DL_to_temp[DL]);
    }
    unsigned LutCalibration::tempToRawDL(unsigned temp, int ti) const
    {

        return 0;
    }
    unsigned LutCalibration::tempToRawDLF(float temp, int ti) const
    {
        return 0;
    }

    bool LutCalibration::applyInvert(const unsigned short *T, const unsigned char *IT, unsigned int size, unsigned short *out) const
    {
        return false;
    }

    int LutCalibration::applyInvert(const unsigned short *T, const unsigned char *IT, int index) const
    {
        return 0;
    }

    bool LutCalibration::apply(const unsigned short *DL, const std::vector<float> &inv_emissivities, unsigned int size, unsigned short *out, bool *saturate) const
    {
        for (size_t i = 0; i < size; i++)
        {
            // TI not used but needed for signature
            out[i] = rawDLToTemp(DL[i], 0);
        }

        return true;
    }
    bool LutCalibration::applyF(const unsigned short *DL, const std::vector<float> &inv_emissivities, unsigned int size, float *out, bool *saturate) const
    {
        for (size_t i = 0; i < size; i++)
        {
            // TI not used but needed for signature
            out[i] = rawDLToTempF(DL[i], 0);
        }

        return true;
    }

}

int main(int argc, char *argv)
{
    std::string fname = "lut.csv";
    auto lut = rir::LutCalibration(fname);
    return 0;
}