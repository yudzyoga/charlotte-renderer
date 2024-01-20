#include <lightwave.hpp>

namespace lightwave {

class LineDistanceTracerIntegrator : public SamplingIntegrator {

    int m_depth;
    bool nee;

    float w_screen;      //The line width in screen-space. Notation refers to the paper
    float beta_depth;    //scaler factor for the depth threshold of linemetric. Notation refers to paper

    float tao_albedo;    //threshold, square of the albedo difference
    float tao_normal;    //threshold
    int   line_samplecount;   //number of line samples for each rays, 16-32 is recommended wrt the paper

public:
    LineDistanceTracerIntegrator(const Properties &properties)
    : SamplingIntegrator(properties) {
        max_distance = properties.get<float>("maxDistance", 40.f);
        m_depth = properties.get<int>("depth", 2);
        nee     = properties.get<bool>("nee", true);

        w_screen   = properties.get<float>("wscreen", 0.15f);  //this, combined with the pixel width in instance, determines the line width in screen-space. Notation refers to the paper
        beta_depth = properties.get<float>("beta", 2.5f);      //default value follow that in the paper
        tao_albedo = properties.get<float>("albedo", 0.05f); //mind that this is the square of the albedo difference, will be quite small
        tao_normal = properties.get<float>("normal", 0.5f); //range in [0,1], the smaller the more strict
        line_samplecount = properties.get<int>("samplecount", 8);
    }
    
    bool isLine_Metric(const Intersection &sample_its, const Intersection &target_its, const Color &target_albedo, Sampler &rng){
        if (sample_its.instance->id() != target_its.instance->id())                                return true;  //check if the sample point is on the same object
        else if((sample_its.sampleBsdf(rng).weight - target_albedo).lengthSquared() > tao_albedo ) return true; //check if the sample point is on the same material/ color region, take the square as metric for saving computation cost
        else if((1.f - abs(sample_its.frame.normal.dot(target_its.frame.normal))) > tao_normal)    return true; //check if the sample point is on the same surface, take the square as metric for saving computation cost
        // else { //check if the sample point is on similar depth, equation refers to the paper
        //     if (sample_its.t < target_its.t) {
        //         if(abs(sample_its.t - target_its.t) > beta_depth * sample_its.t * (sample_its.position - target_its.position).length() / abs(sample_its.t * sample_its.frame.normal.dot(sample_its.wo))) return true;
        //         else return false;
        //     }
        //     else {
        //         if(abs(sample_its.t - target_its.t) > beta_depth * target_its.t * (sample_its.position - target_its.position).length() / abs(sample_its.t * target_its.frame.normal.dot(sample_its.wo))) return true;
        //         else return false;
        //     }

        // }
        return false;
    }

    bool isLine(const Ray &ray, const Intersection &target_its, Sampler &rng, float &prev_w_scaled){
        /*
        step1: find the sample point on the tangent plane of hit point
        step2: create a sample ray, find the intersection
        step3: check if the two intersections(target, sample) fulfill the line metric
        ps. steap2 and step3 are in a loop(we need several samples to make sure the line is sharp)
        */

        //define the radius of the sample area on the tangent plane, getting wider as bouncing deeper
        float w_scaled = w_screen * target_its.instance->getPWidth() * target_its.t + prev_w_scaled; 
        prev_w_scaled  = w_scaled;

        //for saving computation cost, later we only compute the albedo of the sample point
        Color target_albedo = target_its.sampleBsdf(rng).weight;
        
        for(int i = 0; i < line_samplecount; i++){
            //generate a random point on the tangent plane
            auto random        = rng.next2D();
            float random_theta = random[0] * Pi * 2;
            //generate the sample ray based on the random point
            Vector world_pq    = target_its.frame.toWorld(w_scaled * random[1] * Vector(cos(random_theta ),sin(random_theta ),0.f)) ; //find the sample point on the tangent plane of hit point
            Ray sampleRay      = Ray(ray.origin + 0.01f * ray.direction, ray(target_its.t) + world_pq - ray.origin).normalized();  //make sure no self-intersection

            Intersection sample_its = m_scene->intersect(sampleRay, rng);
            if(!sample_its) return true; //no intersection, not a line
            if(isLine_Metric(sample_its, target_its, target_albedo, rng)) return true;
        }

        return false;

    }

    Color Li(const Ray &ray, Sampler &rng) override {
        Intersection its = m_scene->intersect(ray, rng);
        // if skip part of the image, just to speed up rendering
        if (skip_partial && ray.depth==0) {
            if ((ray.direction.x() < -0.8f)){// || (ray.direction.x() > -0.6f)) {
                return Color(0.f);
            }
            if (ray.direction.y() < -0.8f) {
                return Color(0.f);
            }
            // if ((ray.direction.z() > -0.15f)) {
            //     return Color(0.f);
            // }
        }

        // if show any distance
        if (show_distance) {
            if (its) {
                return fog_color * (its.t / max_distance);
            } else {
                return fog_color;
            }

        // if path tracing
        } else {
            // next event estimation for light       
            Color light = Color(0.f);
            float distance = 0.f;
            

            // --------------------------------
            assert(m_depth >= 2);
            if(!m_scene->hasLights()) nee = false;
            /*
            incremental update:
            prev_le     <- prev_le + prev_weight * new_le
            prev_weight <- prev_weight * new_weight
            */
            Color prev_weight = Color(1.f);
            Color prev_le     = Color(0.f);
            Color new_li      = Color(0.f);
            Ray   bounceRay   = ray; //with depth ==0
            float prev_wscaled= 0.f;

            while(bounceRay.depth < m_depth){
                // update ray and its depth
                // every event check light(nne)            
                Intersection its = m_scene->intersect(bounceRay, rng);
                if(!its) {
                    // only break when escape the scene
                    new_li = m_scene->evaluateBackground(bounceRay.direction).value;
                    break;
                } 
                else{
                    if (bounceRay.depth == 0) {
                        distance = its.t;
                    }
                    //check if it's a feature line first
                    if(its.instance->getLinetracer()){
                        if(isLine(bounceRay,its, rng, prev_wscaled)) {
                        assert(prev_wscaled!=0.f);  
                        new_li = its.instance->getLineColor() * Pi * sqr(prev_wscaled); //for compensation of the illumination
                        break;
                        }
                    }

                    Color new_le      = Color(0.f);
                    Color new_weight  = Color(1.f);

                    if(nee && bounceRay.depth < m_depth-1){
                        // nne is recognized as a new bounce, for last bounce, nne is not considered
                        LightSample light_sample = m_scene->sampleLight(rng);

                        if(!light_sample.light->canBeIntersected()) { 
                            DirectLightSample direct_light_sample = light_sample.light->sampleDirect(its.position, rng);
                            bool              light_its           = m_scene->intersect( Ray(its.position, direct_light_sample.wi).normalized(),direct_light_sample.distance,rng);

                            if(!light_its)
                            {
                                BsdfEval bsdf_eval                = its.evaluateBsdf(direct_light_sample.wi);
                                         new_le                  += direct_light_sample.weight * bsdf_eval.value / light_sample.probability;
                            }
                        }
                    }

                    if(its.instance->emission()) new_le += its.evaluateEmission(); //continue tracing
                    
                    //sample next ray direction
                    auto sample_result = its.sampleBsdf(rng);
                    new_weight         = sample_result.weight;

                    //incremental update
                    prev_le           += (prev_weight * new_le);
                    prev_weight       *= new_weight;   

                    if(bounceRay.depth >= m_depth-1) break; //no need to update bounceRay, shortcut

                    //update bounceRay for next iteration, depth +1
                    bounceRay = Ray(its.position, sample_result.wi, bounceRay.depth+1).normalized();             
                }
            }
            //initial values: prev_le=0, prev_weight=1, new_li=0
            light = prev_le + prev_weight * new_li;
            // --------------------------------

            // randomize color
            if (use_fog) {
                if (rng.next() > (distance/max_distance)) {
                    return light;
                } else {
                    return (distance/max_distance) * fog_color;
                }
            } else {
                return light;
            }
        }
    }

    /// @brief An optional textual representation of this class, which can be useful for debugging. 
    std::string toString() const override {
        return tfm::format(
            "LineDistanceTracerIntegrator[\n"
            "  depth = %s,\n"
            "]",
            indent(max_distance)
        );
    }

private:
    float max_distance; //50 halton, 30 independent
    // Color fog_color = Color(0.012,0.042,0.044);
    Color fog_color = Color(0.12,0.42,0.44);
    bool show_distance=false;
    bool skip_partial=false;
    bool use_fog=false;

};

}

// this informs lightwave to use our class NormalIntegrator whenever a <integrator type="normals" /> is found in a scene file
REGISTER_INTEGRATOR(LineDistanceTracerIntegrator, "linedistancetracer")
