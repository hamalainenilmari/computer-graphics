#pragma once

#include <string>
#include <vector>

using namespace std;

struct Args
{
	Args(const vector<string>& args);
	Args() {}
	void parse(const vector<string>& args);

	// Rendering output

	string  input_file;
	string  output_file;
	string  depth_file;
	string  normals_file;
	int		width                   = 100;
	int		height                  = 100;
	bool	stats                   = false;

	// Rendering options

	float	depth_min               = 0.0f;
	float	depth_max               = 1.0f;
	int		bounces                 = 0;
    bool	transparent_shadows     = false;
	bool	shadows                 = false;
	bool	shade_back              = false;
	bool	display_uv              = false;

	// Supersampling

	int	samples_per_pixel           = 1;
    int random_seed                 = 0;

    // Parallelism

    enum SamplePatternType
    {
        Pattern_Regular,            // regular grid within the pixel
        Pattern_UniformRandom,      // uniformly distributed random
        Pattern_JitteredRandom      // jittered within subpixels
    };
    SamplePatternType sampling_pattern = Pattern_Regular;

	enum ReconstructionFilterType
    {
		Filter_Box		= 0,
		Filter_Tent		= 1,
		Filter_Gaussian	= 2
	};
    ReconstructionFilterType reconstruction_filter  = Filter_Box;
	float filter_radius                             = 0.5f;

	// GUI options
	bool	gui             = false;
    bool	show_progress   = true;
};

