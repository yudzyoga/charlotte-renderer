#if __has_include(<OpenImageDenoise/oidn.h>)
#include <lightwave.hpp>
#include <OpenImageDenoise/oidn.h>

namespace lightwave {

class Denoising : public Postprocess {
    OIDNFilter filter;
    OIDNDevice device;
    ref<Image> m_normal;
    ref<Image> m_albedo;
    bool use_normal=false;

public: 
    Denoising(const Properties &properties)
    : Postprocess(properties) {
        device = oidnNewDevice(OIDN_DEVICE_TYPE_CPU);
        oidnCommitDevice(device);

        // Create a filter for denoising a beauty (color) image using optional auxiliary images too
        // This can be an expensive operation, so try not to create a new filter for every image!
        filter = oidnNewFilter(device, "RT"); // generic ray tracing filter

        if (properties.has("normal")) {
            m_normal = properties.get<Image>("normal");  
            m_albedo = properties.get<Image>("albedo");  
            use_normal = true;     
        }
    }
 
    void execute() override {
        Point2i res = m_input->resolution();
        m_output->initialize(res);
        // m_albedo = m_input;

        // Create buffers for input/output images accessible by both host (CPU) and device (CPU/GPU)
        // OIDNBuffer colorBuf  = oidnNewBuffer(device, res.x() * res.y() * 3 * sizeof(float));
        OIDNBuffer inputBuf = oidnNewSharedBuffer(device, m_input->data(), res.x() * res.y() * 3 * m_input->getBytesPerPixel());//sizeof(float));
        OIDNBuffer albedoBuf = oidnNewSharedBuffer(device, m_albedo->data(), res.x() * res.y() * 3 * m_albedo->getBytesPerPixel());//sizeof(float));
        OIDNBuffer outputBuf = oidnNewSharedBuffer(device, m_output->data(), res.x() * res.y() * 3 * m_output->getBytesPerPixel());//sizeof(float));
        OIDNBuffer normalBuf = oidnNewSharedBuffer(device, m_normal->data(), res.x() * res.y() * 3 * m_normal->getBytesPerPixel());//sizeof(float));
        oidnSetFilterImage(filter, "color",  inputBuf, OIDN_FORMAT_FLOAT3, res.x(), res.y(), 0, 0, 0); // beauty
        oidnSetFilterImage(filter, "output",  outputBuf, OIDN_FORMAT_FLOAT3, res.x(), res.y(), 0, 0, 0); // beauty
        if (use_normal) {
            oidnSetFilterImage(filter, "normal",  normalBuf, OIDN_FORMAT_FLOAT3, res.x(), res.y(), 0, 0, 0); // beauty
            oidnSetFilterImage(filter, "albedo",  albedoBuf, OIDN_FORMAT_FLOAT3, res.x(), res.y(), 0, 0, 0); // beauty
        }
        oidnCommitFilter(filter);
        
        // Filter the beauty image
        oidnExecuteFilter(filter);

        // Check for errors
        const char* errorMessage;
        if (oidnGetDeviceError(device, &errorMessage) != OIDN_ERROR_NONE)
            printf("Error: %s\n", errorMessage);

        // Cleanup
        oidnReleaseBuffer(inputBuf);
        oidnReleaseBuffer(outputBuf);
        if (use_normal) {
            oidnReleaseBuffer(normalBuf);
            oidnReleaseBuffer(albedoBuf);
        }

        oidnReleaseFilter(filter);
        oidnReleaseDevice(device);

        // stream out result
        Streaming stream { *m_output };
        stream.update();
    }

    std::string toString() const override {
        return tfm::format("Denoising");
    }
};

}

REGISTER_CLASS(Denoising, "postprocess", "denoising")
#endif