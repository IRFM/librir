
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
    };
    LutCalibration::LutCalibration(const std::string &lut)
    {
        m_data = new PrivateData();
    }
    LutCalibration::LutCalibration() { m_data = new PrivateData(); }
    bool LutCalibration::isValid() const
    {
        return false;
    }

    void LutCalibration::setLutFilename(std::string lut_filename)
    {
        m_data->lut_filename = lut_filename;
    }
    std::string LutCalibration::error() const { return m_data->error; }
    std::string LutCalibration::warning() const { return m_data->warning; }
    std::string LutCalibration::lutFilename() const { return m_data->lut_filename; }

    unsigned LutCalibration::rawDLToTemp(unsigned DL, int ti) const
    {
        return 0;
    }
    float LutCalibration::rawDLToTempF(unsigned DL, int ti) const
    {
        return 0;
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
        return false;
    }
    bool LutCalibration::applyF(const unsigned short *DL, const std::vector<float> &inv_emissivities, unsigned int size, float *out, bool *saturate) const
    {
        return false;
    }
}