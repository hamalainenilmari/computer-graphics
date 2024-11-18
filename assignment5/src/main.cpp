// Include libraries
#include "glad/gl_core_33.h"                // OpenGL
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>             // Window manager
#include <imgui.h>                  // GUI Library
#include <imgui_impl_glfw.h>
#include "imgui_impl_opengl3.h"

#include <Eigen/Dense>              // Linear algebra
#include <Eigen/Geometry>

using namespace Eigen;
using namespace std;

#include <atomic>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <iterator>
#include <numeric>
#include <memory>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <execution>
#include <set>
#include <omp.h>

#include "vec_utils.h"
#include "film.h"
#include "app.h"
#include "camera.h"
#include "ray.h"
#include "hit.h"
#include "scene_parser.h"
#include "args.h"
#include "light.h"
#include "material.h"
#include "object.h"
#include "ray_tracer.h"
#include "sampler.h"
#include "filter.h"

shared_ptr<Image4f> render(RayTracer& ray_tracer, SceneParser& scene, const Args& args, bool parallelize);

// The raytracer in this assignment is a command line application.
// While working on the assignment, if you want to run the raytracer from within Visual
// Studio, use the default argument string in main() to enter the arguments you want.
// For repeated tests, use test scripts like the ones we provide in /exe.
// These can be run simply by double-clicking them in Windows Explorer.
// Hint: if you need to see text output from the raytracer while running it under VS,
// one convenient way is to run it without debugging (Ctrl+F5). VS will then keep the
// console window open for you afterwards.
int main(int argcp, char *argvp[]) {
    if (argcp == 1)
    {
        // no command line arguments; launch interactive viewer
        App* app = new App;
        app->run();
        return 0;
    }

    auto arg = vector<string>(argvp + 1, argvp + argcp);
    // Parse the arguments
    auto args = Args(arg);
    // Parse the scene
    auto scene_parser = SceneParser(args.input_file.c_str());
    // Construct tracer
    auto ray_tracer = RayTracer(scene_parser, args);

    // If there is no scene, just display the UV coords
    if (!scene_parser.getGroup())
        args.display_uv = true;

    // Render; measure time
    auto start = chrono::steady_clock::now();
    render(ray_tracer, scene_parser, args, true);
    auto end = chrono::steady_clock::now();

    cout << "Rendered " << args.output_file << " in " << chrono::duration_cast<chrono::milliseconds>(end-start).count() << "ms." << endl;
    return 0;
}

// Actual renderer, called by both the command line and the interactive application.
// Pass num_threads == 0 to use maximum supported number.
shared_ptr<Image4f> render(RayTracer& ray_tracer, SceneParser& scene, const Args& args, bool parallelize)
{
    auto image_size = Vector2i(args.width, args.height);
    float fAspect = float(args.width) / args.height;

    // Construct images
    shared_ptr<Image4f> color_image, normal_image, depth_image;

    color_image = make_shared<Image4f>(image_size, Vector4f::Zero());

    if (!args.depth_file.empty())
        depth_image = make_shared<Image4f>(image_size, Vector4f::Zero());

    if (!args.normals_file.empty())
        normal_image = make_shared<Image4f>(image_size, Vector4f::Zero());

    // EXTRA
    // The Filter and Film objects are for implementing smarter supersampling extra credit.
    // The requirements only make use of "box filtering", i.e., taking averages of samples
    // just within each pixel so that each sample only affects one pixel.
    // Uncomment the lines below when you start working on the filtering extra.
    //shared_ptr<Filter> filter(Filter::constructFilter(args.reconstruction_filter, args.filter_radius));
    //Film film_color(color_image, filter);
    //Film film_depth(depth_image, filter);
    //Film film_normal(normal_image, filter);
    //mutex m;  // You need to wrap calls to Film::addSample() with std::lock_guard<std::mutex> guard(m)
    //          // in order not to cause issues with many threads writing to the same pixels at the same time.

    // progress counter (atomic to enable updating from different threads)
    atomic<int> lines_done = 0;

    // Main render loop that operates over the scanlines of the image!
    //      Inner loop over all x positions for a scaline
    //          Generate all the samples
    //          Fire rays and get shaded results
    //          Accumulate into image

    // Set up number of threads as desired.
    int num_threads = 1;
    if (parallelize)
        num_threads = omp_get_num_procs();
    omp_set_num_threads(num_threads);

    cout << "Using " << num_threads << " threads" << endl;

    // Loop over scanlines.
    #pragma omp parallel for
    for (int j = 0; j < args.height; ++j)
    {
        // Print progress info
        if (omp_get_thread_num() == 0 && args.show_progress)
            ::printf("%.2f%% \r", lines_done * 100.0f / image_size(1));

        // Construct sampler for this scanline.
        // Done this way so that we can retain determinism even when running parallel for loops.
        auto sampler = unique_ptr<Sampler>(Sampler::constructSampler(args.sampling_pattern, args.samples_per_pixel, args.random_seed + j));

        // Loop over pixels on a scanline
        for (int i = 0; i < args.width; ++i)
        {
            // When working on R9, use these to accumulate the results before writing to the respective images
            //Vector3f color = Vector3f::Zero();
            //Vector3f normal_color = Vector3f::Zero();
            //float depth_color = 0.0f;
            // Loop through all the samples for this pixel.
            for (int n = 0; n < args.samples_per_pixel; ++n)
            {
                // Get the offset of the sample inside the pixel. 
                // You need to fill in the implementation for this function when implementing supersampling.
                // The starter implementation only supports one sample per pixel through the pixel center.
                Vector2f subpixel_offset = sampler->getSamplePosition(n);
                Vector2f pixel_coordinates = Vector2f(float(i), float(j)) + subpixel_offset;

                // Convert floating-point pixel coordinate to canonical view coordinates in [-1,1]^2
                // You need to fill in the implementation for Camera::normalizedImageCoordinateFromPixelCoordinate.
                Vector2f normalized_image_coordinates = Camera::normalizedImageCoordinateFromPixelCoordinate(pixel_coordinates, image_size);

                // Generate the ray using the view coordinates
                // You need to fill in the implementation for this function.
                Ray r = scene.getCamera()->generateRay(normalized_image_coordinates, fAspect);

                // Trace the ray!
                Hit hit;
                float tmin = scene.getCamera()->getTMin();

                // You should fill in the gaps in the implementation of traceRay().
                // args.bounces gives the maximum number of reflections/refractions that should be traced.
                Vector3f sample_color = ray_tracer.traceRay(r, tmin, args.bounces, 1.0f, hit, Vector3f::Ones());

                // YOUR CODE HERE (R0)
                // If args.display_uv is true, we want to render a test UV image where the color of each pixel
                // is a simple function of its position in the image. The red component should linearly increase
                // from 0 to 1 with the x coordinate increasing from 0 to args.width. Likewise the green component
                // should linearly increase from 0 to 1 as the y coordinate increases from 0 to args.height. Since
                // our image is two-dimensional we can't map blue to a simple linear function and just set it to 1.

                //if (args.display_uv)
                //	sample_color = ...


                // YOUR CODE HERE (R9)
                // This starter code only supports one sample per pixel and consequently directly
                // puts the returned color to the image. You should extend this code to handle
                // multiple samples per pixel. Also sample the depth and normal visualization like the color.
                // The requirement is just to take an average of all the samples within the pixel
                // (so-called "box filtering"). Note that this starter code does not take an average,
                // it just assumes the first and only sample is the final color.

                // For extra credit, you can implement more sophisticated ones, such as "tent" and bicubic
                // "Mitchell-Netravali" filters. This requires you to implement the addSample()
                // function in the Film class and use it instead of directly setting pixel values in the image.

                Vector4f s;
                s << sample_color, 1.0f;
                color_image->pixel(i, j) = s;
                if (depth_image)
                {
                    // YOUR CODE HERE (R2)
                    // Here you should linearly map the t range [depth_min, depth_max] to the inverted range [1,0] for visualization
                    // Note the inversion; closer objects should appear brighter.
                    float f = 0.0f;     // change this
                    depth_image->pixel(i, j) = Vector4f{ f, f, f, 1.0f };
                }
                if (normal_image)
                {
                    Vector3f normal = hit.normal;
                    Vector3f col(fabs(normal[0]), fabs(normal[1]), fabs(normal[2]));
                    col = clip(col, Vector3f::Zero(), Vector3f::Ones());
                    normal_image->pixel(i, j) = Vector4f{ col(0), col(1), col(2), 1.0f };
                }
            }
        }
        ++lines_done;
    }

    // YOUR CODE HERE (EXTRA)
    // When working on the better antialias filtering, the
    // colors need to be normalized by dividing by the last channel.
    // Uncomment when you get there.
    //if (color_image)
    //    film_color.normalize_weights();
    //if (depth_image)
    //    film_depth.normalize_weights();
    //if (normal_image)
    //    film_normal.normalize_weights();

    if (!args.output_file.empty())
        color_image->exportPNG(args.output_file);

    if (depth_image && !args.depth_file.empty())
        depth_image->exportPNG(args.depth_file);
    
    if (normal_image && !args.normals_file.empty())
    	normal_image->exportPNG(args.normals_file);

    return color_image;
}
