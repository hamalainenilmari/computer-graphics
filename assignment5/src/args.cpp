#include "args.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

using namespace std;

Args::Args(const vector<string>& args)
{
	parse(args);
}

void Args::parse(const vector<string>& args)
{
    bool samples_set = false;
    bool filter_set = false;
	auto it = args.begin();
	while (it != end(args))
    {
		// Rendering output
		if (*it == "-input") {
			input_file = *++it;
		} else if (*it == "-output") {
			output_file = *++it;
		} else if (*it == "-normals") {
			normals_file = *++it;
		} else if (*it == "-size") {
			width = stoi(*++it);
			height = stoi(*++it);
		} else if (*it == "-stats") {
			stats = true;
		}
		// Rendering options
		else if (*it == "-depth") {
			depth_min = stof(*++it);
			depth_max = stof(*++it);
			depth_file = *++it;
		} else if (*it == "-bounces") {
			bounces = stoi(*++it);
		} else if (*it == "-transparent_shadows") {
			shadows = true;
			transparent_shadows = true;
		} else if (*it == "-shadows") {
			shadows = true;
		} else if (*it == "-shade_back") {
			shade_back = true;
		} else if (*it == "-uv") {
			display_uv = true;
		}
		// Supersampling
		else if (*it == "-uniform_samples") {
            if (samples_set)
                cerr << "Warning: -uniform_samples specified though #samples already set" << endl;
			sampling_pattern = Pattern_UniformRandom;
			samples_per_pixel = stoi(*++it);
            samples_set = true;
		} else if (*it == "-regular_samples") {
            if (samples_set)
                cerr << "Warning: -regular_samples specified though #samples already set" << endl;
            sampling_pattern = Pattern_Regular;
            samples_per_pixel = stoi(*++it);
            samples_set = true;
        } else if (*it == "-jittered_samples") {
            if (samples_set)
                cerr << "Warning: -jittered_samples specified though #samples already set" << endl;
            sampling_pattern = Pattern_JitteredRandom;
            samples_per_pixel = stoi(*++it);
            samples_set = true;
        } else if (*it == "-box_filter") {
            if (filter_set)
                cerr << "Warning: -box_filter specified though filter already set" << endl;
            reconstruction_filter = Filter_Box;
			filter_radius = stof(*++it);
            filter_set = true;
		} else if (*it == "-tent_filter") {
            if (filter_set)
                cerr << "Warning: -box_filter specified though filter already set" << endl;
            reconstruction_filter = Filter_Tent;
			filter_radius = stof(*++it);
            filter_set = true;
        } else if (*it == "-gaussian_filter") {
            if (filter_set)
                cerr << "Warning: -box_filter specified though filter already set" << endl;
            reconstruction_filter = Filter_Gaussian;
			filter_radius = stof(*++it);
            filter_set = true;
        }
		// GUI options
		else if (*it == "-gui") {
			gui = true;
		} else if (*it == "-tessellation") {
            cerr << "Warning: unused option -tessellation" << endl;
			//sphere_horiz = stoi(*++it);
			//sphere_vert = stoi(*++it);
		} else if (*it == "-gouraud") {
            cerr << "Warning: unused option -gouraud" << endl;
            //gouraud_shading = true;
		} else if (*it == "-specular_fix") {
            cerr << "Warning: unused option -specular_fix" << endl;
            //specular_fix = true;
		} else if (*it == "-show_progress") {
			show_progress = true;
		}
		else { assert(false && "Unknown argument!"); }
		++it;
	}
}