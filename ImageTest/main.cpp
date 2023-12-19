#include <iostream>;
#include <string>;
#include <fstream>;

#include "opencv2/highgui.hpp";


const std::string IMAGE_PATH = "frames/output_";
const int IMAGE_PATH_NUMBER_LENGTH = 4;
const int WIDTH = 480;
const int HEIGHT = 360;

unsigned int GetColorNumber(cv:: Vec4b pixel)
{
	unsigned int color_number = 0;
	color_number += (int)pixel[0];
	color_number += (int)pixel[1] << 8;
	color_number += (int)pixel[2] << 16;
	color_number += (int)pixel[3] << 24;
	return color_number;
}

std::string GetAdress(int number)
{
	std::string full_path = IMAGE_PATH;
	std::string number_text = std::to_string(number);
	int number_length = number_text.length();

	for (int i = 0; i < IMAGE_PATH_NUMBER_LENGTH - number_length; i++)
	{
		full_path += "0";
	}

	full_path += number_text;
	full_path += ".jpg";

	return full_path;
}

std::string IntToHex(int value)
{
	std::stringstream stream;
	stream << std::hex << value;
	return stream.str();
}

template <typename Message>void Log(Message message) {
	std::cout << message << std::endl;
}

template <typename Message>void LogFast(Message message)
{
	std::cout << message << "\n";
}
class ColorData
{
public:
	int _color;
	std::vector<std::string> _positions;

	ColorData(int color)
	{
		_color = color;
	}

	void AddStartAndEnd(int start_position, int end_position)
	{
		std::string position_couple;
		position_couple =  IntToHex(start_position) + "/" + IntToHex(end_position);
		_positions.push_back(position_couple);
	}

	std::string ToString()
	{
		std::string text = "{" + std::to_string(_color);

		for (std::string &position_couple : _positions)
		{
			text += ", '" + position_couple + "'";
		}

		text += "}";

		return text;
	}

	int GetSize()
	{
		return _positions.size() - 1;
	}
};

class FrameData
{
public:
	std::string _order = "xy";
	std::vector<ColorData> _data;

	void AddColorStartAndEnd(int color, int start_position, int end_position)
	{
		for (ColorData &existing_data : _data)
		{
			if (existing_data._color != color) {
				continue;
			}
			existing_data.AddStartAndEnd(start_position, end_position);
			return;
		}
		ColorData new_data = ColorData(color);
		new_data.AddStartAndEnd(start_position, end_position);
		_data.push_back(new_data);
	}

	std::string ToString()
	{
		std::string text = "{ '" + _order + "', ";
		for (ColorData &data : _data)
		{
			text += data.ToString();
			text += ", ";
		}
		text += "}";
		return text;
	}

	int GetSize()
	{
		int total_size = 0;
		for (ColorData& data : _data)
		{
			total_size += data.GetSize();
		}
		return total_size;
	}
};

class VideoData
{
public:
	std::vector<FrameData> _data;
	void AddFrameData(FrameData& frame_data)
	{
		_data.push_back(frame_data);
	}

	std::string ToString()
	{
		std::string text = "{";
		for (FrameData& frame_data : _data)
		{
			text += frame_data.ToString() + ", ";
		}
		text += "}";

		return text;
	}

	int GetSize()
	{
		return _data.size();
	}
};

int SnapPixelValue(uchar & pixel)
{	
	/*
	 3 color values
	*/
	return pixel > 170 ? 2 : pixel > 30 ? 1 : 0;

	/*
	 2 color values
	*/
	//return pixel > 30 ? 1 : 0;
}

void ApplyFrameChanges(cv::Mat& image, std::vector<std::vector<int>>& whole_picture)
{
	using namespace cv;
	cv::Size image_size = image.size();

	for (int y = 0; y < image_size.height; y++)
	{
		for (int x = 0; x < image_size.width; x++)
		{
			uchar& pixel = image.at<uchar>(y, x);
			whole_picture[y][x] = SnapPixelValue(pixel);
		}
	}
	return;
}

void GenerateFrameDataByOrder(FrameData& frame_data, cv::Mat & image, std::vector<std::vector<int>>& whole_picture, std::string order)
{
	cv:: Size image_size = image.size();

	int start = 0;
	int end = 0;
	int current_color = -1;

	frame_data._order = order;

	if (order == "xy")
	{
		for (int y = 0; y < image_size.height; y++)
		{
			for (int x = 0; x < image_size.width; x++)
			{
				int pixel = SnapPixelValue(image.at<uchar>(y, x));
				int& comarison_pixel = whole_picture[y][x];
				/*
				check if the pixel the same in the last frame
				*/
				if (comarison_pixel == pixel)
				{
					/*
					saves the last pixel data to frame data if it is not nil
					*/
					if (current_color != -1)
					{
						frame_data.AddColorStartAndEnd(current_color, start, end);
					}
					current_color = -1;
					continue;
				}
				/*
				on pixel change saves the pixel data
				*/
				if (current_color != pixel)
				{
					if (current_color != -1)
					{
						frame_data.AddColorStartAndEnd(current_color, start, end);
					}
					current_color = pixel;
					start = y * WIDTH + x;
				};
				/*
				if xy end = y*WIDTH + x else end = HEIGHT*x+y
				to loop through each row or each column
				*/
				end = y * WIDTH + x ;
			}
		}
		if (current_color != -1) {
			frame_data.AddColorStartAndEnd(current_color, start, end);
		}
		return;
	}
	
	for (int x = 0; x < image_size.width; x++)	
	{
		for (int y = 0; y < image_size.height; y++)
		{
			int pixel = SnapPixelValue(image.at<uchar>(y, x));
			int& comarison_pixel = whole_picture[y][x];
			
			if (comarison_pixel == pixel)
			{
				
				if (current_color != -1)
				{
					frame_data.AddColorStartAndEnd(current_color, start, end);
				}
				current_color = -1;
				continue;
			}
			if (current_color != pixel)
			{
				if (current_color != -1)
				{
					frame_data.AddColorStartAndEnd(current_color, start, end);
				}
				current_color = pixel;
				start = HEIGHT * x + y;
			};
			end = HEIGHT * x + y;
		}
	}
	if (current_color != -1) {
		frame_data.AddColorStartAndEnd(current_color, start, end);
	}
}

FrameData GenerateFrameData(int image_number, std::vector<std::vector<int>> &whole_picture) {
	using namespace cv;
	std::string image_path = GetAdress(image_number);
	Mat image = cv::imread(image_path, 0);
	

	FrameData frame_data_xy;
	FrameData frame_data_yx;

	/*
	generate 2 FrameData in different order
	xy checks each pixel in row
	yx checks each pixel in column
	*/
	GenerateFrameDataByOrder(frame_data_xy, image, whole_picture, "xy");
	GenerateFrameDataByOrder(frame_data_yx, image, whole_picture, "yx");

	/*
	applies changes to the frame
	*/
	ApplyFrameChanges(image, whole_picture);
	
	/*
	test for size optimisation
	*/

	//Log(frame_data_xy.GetSize());
	//Log(frame_data_yx.GetSize());
	LogFast(image_number);

	/*
	returns the frame with the less data
	*/
	return frame_data_xy.GetSize() < frame_data_yx.GetSize() ? frame_data_xy : frame_data_yx;
}
std::vector<std::vector<int>> whole_picture = {};

int main()
{
	/*
	prepearing the file
	*/
	std::fstream file;
	file.open("FrameData.txt", std::ios::out);
	if (!file.is_open())
	{
		Log("File is not open");
		return 0;
	}
	
	/*
	creating a matrix of -1 for comparison
	-1 means no color -> will be used in pixel checks
	*/
	for (int y = 0; y < HEIGHT; y++) 
	{
		std::vector<int> row = {};
		for (int x = 0; x < WIDTH; x++)
		{
			row.push_back(-1);
		}
		whole_picture.push_back(row);
	}

	VideoData video;
	for (int i = 1; i <= 6572; i++)
	{
		FrameData frame = GenerateFrameData(i, whole_picture);
		video.AddFrameData(frame);
	}
	
	/*
		test for amount of data
	*/
	//for (FrameData frame_data : video._data)
	//{
	//	file << frame_data.ToString() << "\n";
	//}
	file << video.ToString();
	Log(video.ToString().size());

	file.close();
	Log("Finished");
	Log(video.GetSize());
	std::cin.get();
	return 0;
}