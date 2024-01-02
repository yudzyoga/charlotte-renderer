#include <lightwave.hpp>

namespace lightwave {

class Bloom : public Executable {
// protected:
    /// @brief The input image that is to be processed.
    ref<Image> m_input;
    /// @brief The output image that will be produced.
    ref<Image> m_output;

    // hyperparams
    int width, iters;
    float limit, sigma, scale;

public: 
    Bloom(const Properties &properties) {
        m_input = properties.get<Image>("input");
        m_output = properties.getChild<Image>();
        width = properties.get<int>("width", 2);
        iters = properties.get<int>("iters", 5);
        limit = properties.get<float>("limit", 0.8f);
        sigma = properties.get<float>("sigma", 2.0f);
        scale = properties.get<float>("scale", 1.0f);
    }
 
    void execute() override {
        runBloom();
        m_output->saveAt("./bloom_output.exr");
    }

    // float gamma = 2.2f;

    // Color getPixelColor(int x, int y){
    //     x = (int)clamp(x, 0, m_input->resolution().x()-1);
    //     y = (int)clamp(y, 0, m_input->resolution().y()-1);
    //     Color color = m_input->get(Point2i{x, y});
    //     return color;
    // }

    void runBloom(){
        
        m_output->initialize(m_input->resolution());
        int nSurvivors = 0;
        Point2i res = m_input->resolution();
        std::unique_ptr<Color[]> thresholded(new Color[res.x() * res.y()]);
        std::vector<std::unique_ptr<Color[]>> blurred;
        // Compute filter weights
        std::vector<float> wts(width, float(0));
        float wtSum = 0;
        int radius = width / 2;
        
        // First, threshold the source image
        for (int i = 0; i < res.x(); ++i) {
            for (int j = 0; j < res.y(); ++j) {
                Point2i pt = {i, j};
                Color rgb = m_input->operator()(pt);
                if (rgb.r() > limit || rgb.g() > limit || rgb.b() > limit) {
                    ++nSurvivors;
                    thresholded[j * res.x() + i] = rgb;
                } else
                    thresholded[j * res.x() + i] = Color::black();    
            }        
        }

        if (nSurvivors == 0) {
            fprintf(stderr,
                    "imgtool: warning: no pixels were above bloom threshold %f\n",
                    limit);
            *m_output->data() = *m_input->data();
            return;
        }
        blurred.push_back(std::move(thresholded));

        if ((width % 2) == 0) {
            ++width;
            fprintf(
                stderr,
                "imgtool: bloom width must be an odd value. Rounding up to %d.\n",
                width);
        }


        for (int i = 0; i < width; ++i) {
            float v = std::abs(float(i - radius)) / float(radius);
            wts[i] = std::exp(-sigma * v);
            wtSum += wts[i];
        }
        // Normalize filter weights.
        for (int i = 0; i < width; ++i) wts[i] /= wtSum;

        auto getTexel = [&](const std::unique_ptr<Color[]> &img, Point2i p) {
            // Clamp at boundaries
            if (p.x() < 0) p.x() = 0;
            if (p.x() >= res.x()) p.x() = res.x() - 1;
            if (p.y() < 0) p.y() = 0;
            if (p.y() >= res.y()) p.y() = res.y() - 1;
            return img[p.y() * res.x() + p.x()];
        };

        // Now successively blur the thresholded image.
        std::unique_ptr<Color[]> blurx(new Color[res.x() * res.y()]);
        for (int iter = 0; iter < iters; ++iter) {
            // Separable blur; first blur in x into blurx
            for (int y = 0; y < res.y(); ++y) {
                for (int x = 0; x < res.x(); ++x) {
                    Color result = Color(0.f);
                    for (int r = -radius; r <= radius; ++r)
                        result += wts[r + radius] * getTexel(blurred.back(), {x + r, y});
                    blurx[y * res.x() + x] = result;
                }
            }

            // Now blur in y from blur x to the result
            std::unique_ptr<Color[]> blury(new Color[res.x() * res.y()]);
            for (int y = 0; y < res.y(); ++y) {
                for (int x = 0; x < res.x(); ++x) {
                    Color result = Color(0.f);
                    for (int r = -radius; r <= radius; ++r)
                        result += wts[r + radius] * getTexel(blurx, {x, y + r});
                    blury[y * res.x() + x] = result;
                }
            }
            blurred.push_back(std::move(blury));
        }

        // Finally, add all of the blurred images, scaled, to the original.
        for (int i = 0; i < res.x() * res.y(); ++i) {
            Color blurredSum = Color(0.f);
            // Skip the thresholded image, since it's already present in the
            // original; just add pixels from the blurred ones.
            for (size_t j = 1; j < blurred.size(); ++j) blurredSum += blurred[j][i];

            Point2i pt = {i % (int)res.x(), (int)floor((float)i / (float)res.x())};
            m_output->operator()(pt) = m_input->operator()(pt) + ((scale / iters) * blurredSum);
        }
        return;
    }   

    std::string id() const {
        return "bloom_feature";
    }

    std::string toString() const override {
        return tfm::format("Bloom");
    }
};

}

REGISTER_CLASS(Bloom, "postprocess", "bloom")