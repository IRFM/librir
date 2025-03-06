#ifndef LUT_CALIBRATION_H
#define LUT_CALIBRATION_H

#include <string>
#include <vector>
#include <map>

#include "rir_config.h"
#include "Misc.h"

namespace rir
{
    class IRVideoLoader;

    typedef std::map<std::string, std::string> dict_type;

    /**
     * Base class for IR image calibration in surface temperature
     */
    class IO_EXPORT LutCalibration : public BaseCalibration
    {

    public:
        enum Features
        {
            SupportGlobalEmissivity = 0x01,
            SupportPixelEmissivity = 0x02,
        };
        LutCalibration(const std::string &lut);
        LutCalibration();
        virtual ~LutCalibration() { delete m_data; };
        virtual std::string name() const { return "LutCalibration"; }

        /** Tells if this calibration object needs calls to prepareCalibration() before applying its calibraion through apply(). */
        virtual bool needPrepareCalibration() const { return false; }
        /** Configure the calibration (and next call to apply()) based on raw image attributes */
        virtual bool prepareCalibration(const dict_type &attributes) { return true; }

        /**Return supported features as a combination of Features enum*/
        virtual int supportedFeatures() const
        {
            return (SupportGlobalEmissivity | SupportPixelEmissivity);
        }

        /** Returns true if this calibration has its initial parmaters */
        virtual bool hasInitialParameters() const
        {
            return true;
        }

        /**Returns true if the calibration is valid, false otherwise*/
        bool isValid() const;
        /**Returns potential warnings (with new line separators) emitted while building the calibration*/
        std::string error() const;
        /**Returns potential errors (with new line separators) emitted while building the calibration*/
        std::string warning() const;
        /** Flip transmission factors. Default implementation does nothing.*/
        virtual bool flipTransmissions(bool fip_lr, bool flip_ud, int width, int height)
        {
            (void)fip_lr;
            (void)flip_ud;
            (void)width;
            (void)height;
            return true;
        }
        /**Returns the lut full file path*/
        std::string lutFilename() const;
        void setLutFilename(std::string lut_filename);
        /**Returns the list of calibration files required for this calibration*/
        virtual StringList calibrationFiles() const
        {
            StringList res;
            res.push_back(lutFilename());

            return res;
        }

        /**Convert Digital Level value and Integration time to temperature (°C) for given integration time*/
        unsigned rawDLToTemp(unsigned DL, int ti) const;
        /**Convert Digital Level value and Integration time to temperature (°C) for given integration time*/
        float rawDLToTempF(unsigned DL, int ti) const;
        /**Convert temperature (°C) to Digital Level for given Integration time*/
        unsigned tempToRawDL(unsigned temp, int ti) const;
        unsigned tempToRawDLF(float temp, int ti) const;

        /**
         * Returns the names of internal used tables (floating point matrices) used for calibration
         */
        virtual StringList tableNames() const { return StringList(); }
        virtual std::pair<const float *, size_t> getTable(const char *name) const { return {nullptr, 0}; }

        /**
        Apply inverted calibration (from T°C to DL).
        IT is the image of integration time (can be NULL for some implementations)
        */
        bool applyInvert(const unsigned short *T, const unsigned char *IT, unsigned int size, unsigned short *out) const;
        int applyInvert(const unsigned short *T, const unsigned char *IT, int index) const;
        /**
        Calibrate input Digital Levels to surface temperature (C).
        @param DL input DL image
        @param inv_emissivities inverted emissivities per pixel
        @param size input image size
        @param out output temperature image
        @param saturate if not NULL, set to true if the calibration saturated, false otherwise
        Returns true on success, false otherwise.
        */
        bool apply(const unsigned short *DL, const std::vector<float> &inv_emissivities, unsigned int size, unsigned short *out, bool *saturate = NULL) const;

        /**
        Calibrate input Digital Levels to temperature.
        @param DL input DL image
        @param inv_emissivities inverted emissivities per pixel
        @param size input image size
        @param out output temperature image
        @param saturate if not NULL, set to true if the calibration saturated, false otherwise
        Returns true on success, false otherwise.

        This function performs a floating point calibration (precision < 1°C) and is slightly slower than #apply() function.
        */
        bool applyF(const unsigned short *DL, const std::vector<float> &inv_emissivities, unsigned int size, float *out, bool *saturate = NULL) const;

    private:
        class PrivateData;
        PrivateData *m_data;
    };

}

#endif // LUT_CALIBRATION_H