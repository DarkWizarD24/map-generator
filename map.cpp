#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

//---------------------------------------------------------------//
//                             Types                             //
//---------------------------------------------------------------//

//! Contain a RGB color
struct Color
{
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

//---------------------------------------------------------------//
//                        Random numbers                         //
//---------------------------------------------------------------//

//! Provide a random number between min and max value
uint32_t randr(uint32_t min, uint32_t max)
{
  double scaled = (double)rand() / RAND_MAX;

  return (max - min + 1) * scaled + min;
}

//---------------------------------------------------------------//
//                        Miscellaneous                          //
//---------------------------------------------------------------//

//! Return true if the number is a power of two
bool is_power_of_two(uint32_t x)
{
  return ((x != 0) and !(x & (x - 1)));
}

//! Average between 4 unsigned values. If some of the values are negative, their are ignored.
uint16_t average(int32_t v1, int32_t v2, int32_t v3, int32_t v4)
{
  uint32_t sum = 0;
  uint8_t divisor = 0;

  if (v1 >= 0)
  {
    sum += v1;
    divisor++;
  }

  if (v2 >= 0)
  {
    sum += v2;
    divisor++;
  }

  if (v3 >= 0)
  {
    sum += v3;
    divisor++;
  }

  if (v4 >= 0)
  {
    sum += v4;
    divisor++;
  }

  return sum / divisor;
}

//---------------------------------------------------------------//
//                            Spinner                            //
//---------------------------------------------------------------//

//! Manage the spinner that can be displayed in the shell to indicated to the user that an operation is ongoing.
class Spinner
{
  public :
    //! Add the spinner to the shell
    static void add()
    {
      printf("|");
      spin = 1;
    }

    //! Update the spinner with the next step
    static void update()
    {
      switch (spin)
      {
        case 0 : printf("\b|");  spin = 1; break;
        case 1 : printf("\b/");  spin = 2; break;
        case 2 : printf("\b-");  spin = 3; break;
        case 3 : printf("\b\\"); spin = 4; break;
        case 4 : printf("\b|");  spin = 5; break;
        case 5 : printf("\b\\"); spin = 6; break;
        case 6 : printf("\b-");  spin = 7; break;
        case 7 : printf("\b/");  spin = 0; break;
      }
    }

    //! Remove the spinner from the shell
    static void remove()
    {
      printf("\b");
    }
  private :
    //! Use to mesmerize the spin animation state
    static uint8_t spin;
};

uint8_t Spinner::spin = 0;

//---------------------------------------------------------------//
//                         Configuration                         //
//---------------------------------------------------------------//

//! Store the configuration of all the algorithme
//! This class is a singleton and can be access by any part of the software on created
//! /todo load this parameter from a json file to allow to save configurations
class Config
{
  public :
    inline static Config & get()
    {
      static Config singleton;
      return singleton;
    }

    //! Used to initialize the random number generator, each seed provide an unique map
    uint32_t seed = 2344544;

    //! Size of the map in pixel (with & height)
    uint32_t map_size = 2048;

    //! Eleveation of the corners [0, 65535]
    uint32_t left_top_corner_height = 65535 * 1;
    uint32_t right_top_corner_height = 65535 * 1;
    uint32_t left_bottom_corner_height = 65535 * 1;
    uint32_t right_bottom_corner_height = 65535 * 1;

    //! Heigh of the ocean [0, 65535]
    uint32_t ocean_height = 65535 * 0.3;

    //! Factor used by the level generator, the lower this value is the flatter the map is
    float roughness = 25;

    //! Maximal number of river spring to be generated
    uint32_t spring_max = 20;
    //! Size of the rivers
    float rivers_size = 0.5;

    //! Factor used when creating the map image, the more this factore the more the relief cast shadow and the relief appeare crispe. Has only a cosmetic effect.
    float light_level = 75.0;

    //! Number of entries in height_colors
    uint8_t height_colors_count = 19;

    //! Colors of the map above sea level
    Color height_colors[19] = {
      {172, 208, 165},
      {148, 191, 139},
      {168, 198, 143},
      {189, 204, 150},
      {209, 215, 171},
      {225, 228, 181},
      {239, 235, 192},
      {232, 225, 182},
      {222, 214, 163},
      {211, 202, 157},
      {202, 185, 130},
      {195, 167, 107},
      {185, 152, 90 },
      {170, 135, 83 },
      {172, 154, 124},
      {186, 174, 154},
      {202, 195, 184},
      {224, 222, 216},
      {245, 244 ,242}
    };

    //! Number of entries in negtive_height_colors
    uint8_t negative_height_colors_count = 10;

     //! Colors of the map under sea level
    Color negative_height_colors[10] = {
      {113, 171, 216},
      {121, 178, 222},
      {132, 185, 227},
      {141, 193, 234},
      {150, 201, 240},
      {161, 210, 247},
      {172, 219, 251},
      {185, 227, 255},
      {198, 236, 255},
      {216, 242, 254}
    };

    Color river_color = {9, 120, 171};

    bool generate_topographic_map = true;

    bool generate_biome_map = true;

    float smooth_factor = 0.95;

    float smooth_pass = 10;

  private:
    Config() { }
    Config(const Config &rhs);
    Config &operator=(const Config &rhs);
};

//---------------------------------------------------------------//
//                       Color Management                        //
//---------------------------------------------------------------//
//! Color picker is used to draw the map, it's convert an height and moisture to a color.
class Color_picker
{
  public :
    virtual Color color(uint16_t height, uint8_t moisture) const = 0;
};

//! Convert an altitude into a color, used to draw topographic maps.
class Topographic_color_picker : public Color_picker
{
  public:   
    //! /param min Minimal height of the map, use to set the deepest color
    //! /param max Minimal height of the map, use to set the highest color
    Topographic_color_picker (uint16_t min = 0, uint16_t max = 65535)
    {    
      printf("Computing topographic colors...");
      
      colors = new Color[65535];
      
      for (uint16_t i = 0 ; i < sizeof(colors) ; i++)
      {
        colors[i] = Color{0, 0, 0};
      }
      
      uint8_t index = 0;
      uint16_t offset = Config::get().ocean_height;
      uint16_t step = (offset - min) / Config::get().negative_height_colors_count;

      Color color_lower = Config::get().negative_height_colors[index];

      for (uint16_t height = min ; height < offset ; height++)
      {               
        if (height > (index + 1) * step)
        {
           color_lower = Config::get().negative_height_colors[++index];
        }
        
        Color color_greater;        
        //when the end of the color list is reach, use the last value as greater value
        if (index + 1 == Config::get().negative_height_colors_count)
        {
          color_greater = color_lower;
        }
        else
        {
          color_greater = Config::get().negative_height_colors[index + 1];
        }
     
        colors[height].red = color_lower.red + double(double(color_greater.red - color_lower.red) / double(step)) * (height - index * step);
        colors[height].green = color_lower.green + double(double(color_greater.green - color_lower.green) / double(step)) * (height - index * step);
        colors[height].blue = color_lower.blue + double(double(color_greater.blue - color_lower.blue) / double(step)) * (height - index * step);
      }
      
      index = 0;
      step = (max - offset) / Config::get().height_colors_count;
      
      color_lower = Config::get().height_colors[index];
      for (uint16_t height = offset ; height < max ; height++)
      {       
        if (height > offset + (index + 1) * step)
        {
          color_lower = Config::get().height_colors[++index];
        }
        
        Color color_greater;
        //when the end of the color list is reach, use the last value as greater value
        if (index + 1 == Config::get().height_colors_count)
        {
          color_greater = color_lower;
        }
        else
        {
          color_greater = Config::get().height_colors[index + 1];
        }
        
        colors[height].red = color_lower.red + double(double(color_greater.red - color_lower.red) / double(step)) * (height - offset - index * step);
        colors[height].green = color_lower.green + double(double(color_greater.green - color_lower.green) / double(step)) * (height - offset - index * step);
        colors[height].blue = color_lower.blue + double(double(color_greater.blue - color_lower.blue) / double(step)) * (height - offset - index * step);
      }      
      printf("done\n");
    }

    ~Topographic_color_picker()
    {
      delete colors;
    }

    //! Topographic map is build only by using the altitude
    Color color(uint16_t height, uint8_t /*moisture*/) const
    {
      if (colors != nullptr)
      {
        return colors[height];
      }
      return Color{0, 0, 0};    
    }
  
  private:   
    Color * colors;
};

//! Use a whittaker diagram to provide color acording to height (temperature) and moisture
class Biome_color_picker : public Color_picker
{
  Color color(uint16_t /*height*/, uint8_t /*moisture*/) const
  {
    //TODO
    return Color{0, 0, 0};
  }
};

//---------------------------------------------------------------//
//                         Map generator                         //
//---------------------------------------------------------------//

class Map
{
  public:
    Map()
    {
      init();
      generate_height();
      height_smooth();
      generate_rivers();
      generate_cities();
      generate_road();
    }
    
    ~Map()
    {
      delete _height;
    }
      
    //! Save the map to a file. Use the color picker to obtain the colors.
    void save(const Color_picker * color_picker, std::string name)
    {
      if (color_picker == nullptr)
      {
        printf("Error : Cannot save map whithout color picker\n");
        exit(1);
      }

      printf("Saving map...");
      Spinner::add();
    
      FILE * fp;

      fp = fopen ((name + ".ppm").c_str(), "w");
      fprintf(fp, "P3\n");
      fprintf(fp, "%d %d\n", size, size);
      fprintf(fp, "255\n");

      for (uint32_t x = 0 ; x < size ; x++)
      {
        //Update spinner only each line to improve performance
        Spinner::update();

        for (uint32_t y = 0 ; y < size ; y++)
        {
          Color color;
          // if the pixel is water (river or lac, use dedicated color, otherwize obtain color from height)
          if (water(x, y) != 0)
          {
            color = Config::get().river_color;
          }
          else
          {
            // Get color from the color picker
            color = color_picker->color(height(x, y), moisture(x, y));
          }

          //The color is altered by the relief
          int32_t west_height = height(x + 1, y);
          int32_t south_height = height(x, y + 1);

          //If the west or south pixel are out of the map, do not change color for this pixel
          if ((west_height != -1) and (south_height != -1))
          {
            int32_t delta = west_height + south_height - 2 * height(x, y);
            float factor = float(delta) / (65535.0 / Config::get().light_level);

            //Reduce the light under water to obtain a better looking result.
            if (height(x, y) < (int32_t)Config::get().ocean_height)
            {
              factor = factor / 3;
            }

            if (delta >= 0)
            {
              color.red = color.red * (1.0 - factor);
              color.green = color.green * (1.0 - factor);
              color.blue = color.blue * (1.0 - factor);
            }
            else
            {
              color.red = color.red + (factor * (255 - color.red));
              color.green = color.green + (factor * (255 - color.green));
              color.blue = color.blue + (factor * (255 - color.blue));
            }
          }
          
          fprintf(fp, "%d %d %d ", color.red, color.green, color.blue);
        }
        fprintf(fp, "\n");
      }

      fclose(fp);
      
      Spinner::remove();
      printf("done\n");
    }

    uint16_t height_max()
    {
      uint16_t max = 0;
      for (uint32_t i = 0 ; i < (uint32_t)(size * size) ; i++)
      {
        if (_height[i] > max)
        {
          max = _height[i];
        }
      }
      return max;
    }

    uint16_t height_min()
    {
      uint16_t min = 65535;
      for (uint32_t i = 0 ; i < (uint32_t)(size * size) ; i++)
      {
        if (_height[i] < min)
        {
          min = _height[i];
        }
      }
      return min;
    }

  private:
    struct Pixel_height
    {
      uint16_t x;
      uint16_t y;
      uint16_t height;
    };

    void init()
    {
      printf("Initializing map generator...");

      //Map size must be a power of two + 1
      uint16_t map_size = Config::get().map_size;

      //If not a power of two, find the nearest power of two by decrementing
      while(not is_power_of_two(map_size))
      {
        map_size--;
      }

      //The + 1
      map_size++;

      size = map_size;

      _height = new uint16_t [size * size];

      //Set to zero the height
      for (uint32_t i = 0 ; i < (uint32_t)(size * size) ; i++)
      {
        _height[i] = 0;
      }

      _water = new uint8_t [size * size];

      //Set to zero the water
      for (uint32_t i = 0 ; i < (uint32_t)(size * size) ; i++)
      {
        _water[i] = 0;
      }

      _moisture = new uint8_t [size * size];

      //Set to zero the water
      for (uint32_t i = 0 ; i < (uint32_t)(size * size) ; i++)
      {
        _moisture[i] = 0;
      }

      printf("done\n");
    }

    //use Diamond-square algorithm to compote the height map
    //http://en.wikipedia.org/wiki/Diamond-square_algorithm
    void generate_height()
    { 
      printf("Computing height map...");
      Spinner::add();
             
      //Set the initial corners
      height(0, 0, Config::get().left_top_corner_height);
      height(0, size-1, Config::get().right_top_corner_height);
      height(size - 1, 0, Config::get().left_bottom_corner_height);
      height(size - 1, size-1, Config::get().right_bottom_corner_height);
      
      //Each step of the algorithme, the map is splitted in smaler squares
      for (int32_t square_size = size ; square_size > 2 ; square_size =  square_size / 2 + 1)
      {      
        Spinner::update();
        //For each squares, compute it's center coordinates
        for (uint32_t x = square_size / 2 ; x < size ; x = x + square_size - 1)
        {         
          for (uint32_t y = square_size / 2 ; y < size ; y = y + square_size - 1)
          {           
            //Random offset
            uint16_t offset = randr(- Config::get().roughness * square_size, Config::get().roughness * square_size);
            
            //center value equal the mean of the square corners
            height(x, y, ( height(x - square_size / 2, y - square_size / 2)
                         + height(x + square_size / 2, y - square_size / 2)
                         + height(x - square_size / 2, y + square_size / 2)
                         + height(x + square_size / 2, y + square_size / 2)) / 4 + offset);
          }
        }
        
        //For each diamond, compute it's center coordinates
        for (int32_t x = 0 ; x < size ; x = x + square_size / 2)
        {
          for (int32_t y = square_size / 2 - x % (square_size - 1) ; y < size ; y = y + square_size - 1)
          {
            //Random offset
            uint16_t offset = randr(- Config::get().roughness * square_size, Config::get().roughness * square_size);
            
            //center value equal the mean of the diamond corners
            int32_t top = -1;
            int32_t right = -1;
            int32_t bottom = -1;
            int32_t left = -1;
            
            if (((x >= 0) and (x < size)) and ((y - square_size / 2 >= 0) and (y - square_size / 2 < size)))
            {
              top = height(x, y - square_size / 2);
            }
            
            if (((x + square_size / 2 >= 0) and (x + square_size / 2 < size)) and ((y >= 0) and (y < size)))
            {
              right = height(x + square_size / 2, y);
            }
            
            if (((x >= 0) and (x < size)) and ((y + square_size / 2 >= 0) and (y + square_size / 2 < size)))
            {
              bottom = height(x, y + square_size / 2);
            }
            
            if (((x - square_size / 2 >= 0) and (x - square_size / 2 < size)) and ((y >= 0) and (y < size)))
            {
              left = height(x - square_size / 2, y);
            }
            
            height(x, y, average(top, right, bottom, left) + offset);
          }
        }
      }
      
      Spinner::remove();
      printf("done\n");
    }

    //! Used by height_smooth
    void height_smooth_pixel(uint16_t current_x, uint16_t current_y, uint16_t neighbor_x, uint16_t neighbor_y)
    {
      int32_t current_height = height(current_x, current_y);
      int32_t neighbor_height = height(neighbor_x, neighbor_y);

      float smooth_factor = Config::get().smooth_factor;

      if (neighbor_height != -1)
      {
        height(current_x, current_y, neighbor_height * (1.0 - smooth_factor) + current_height * smooth_factor);
      }
    }

    //! Smooth the eight of the terrain, more pass are done on water for a more realistic result
    //! Based on http://www.lighthouse3d.com/opengl/terrain/index.php3?smoothing
    void height_smooth()
    {
      printf("Smoothing height map...");
      Spinner::add();

      for (uint8_t pass = 0 ; pass < Config::get().smooth_pass ; pass++)
      {
        // Rows, left to right
        for (uint32_t x = 0 ; x < size ; x++)
        {
          Spinner::update();

          for (uint32_t y = 0 ; y < size ; y++)
          {
            height_smooth_pixel(x, y, x - 1, y);
          }
        }

        // Rows, right to left
        for (uint32_t x = size - 1 ; x <= 0  ; x--)
        {
          Spinner::update();

          for (uint32_t y = 0 ; y < size ; y++)
          {
            height_smooth_pixel(x, y, x + 1, y);
          }
        }

        // Columns, bottom to top
        for (uint32_t x = 0 ; x < size ; x++)
        {
          Spinner::update();

          for (uint32_t y = 0 ; y < size ; y++)
          {
            height_smooth_pixel(x, y, x, y - 1);
          }
        }

        // Columns, top to bottom
        for (uint32_t x = 0 ; x < size ; x++)
        {
          Spinner::update();

          for (uint32_t y = size - 1 ; y <= 0  ; y--)
          {
            height_smooth_pixel(x, y, x, y + 1);
          }
        }
      }

      Spinner::remove();
      printf("done\n");
    }

    void generate_rivers()
    {
      printf("Computing rivers...");
      Spinner::add();

      uint32_t spring_count = randr(0, Config::get().spring_max);

      for (uint32_t i = 0 ; i < spring_count ; i++)
      {
        Spinner::update();

        //Compute the coordinates of the spring
        uint16_t x = randr(0, size);
        uint16_t y = randr(0, size);

        //If the spring is inside the ocean, skip it
        if (height(x, y) < (int32_t)(Config::get().ocean_height))
        {
          continue;
        }

        //TODO std::priority_queue<RiverElement> river;
      }

      Spinner::remove();
      printf("done\n");
    }

    void generate_cities()
    {
      //TODO
    }

    void generate_road()
    {
      //TODO
    }

    //! \return height if x and y are inside the map, -1 otherwise
    int32_t height(int32_t x, int32_t y)
    {
      if ((x < size) and (y < size) and (x >= 0) and (y >= 0))
      {
        return _height[x + size * y];
      }
      else
      {
        return -1;
      }
    }
    
    //! Set height if x and y are inside the map
    void height(int32_t x, int32_t y, uint16_t value)
    {
      if ((x < size) and (y < size) and (x >= 0) and (y >= 0))
      {
        _height[x + size * y] = value;
      }
    }

    //! \return water if x and y are inside the map, -1 otherwise
    int16_t water(int32_t x, int32_t y)
    {
      if ((x < size) and (y < size) and (x >= 0) and (y >= 0))
      {
        return _water[x + size * y];
      }
      else
      {
        return -1;
      }
    }

    //! Set water if x and y are inside the map
    void water(int32_t x, int32_t y, uint8_t value)
    {
      if ((x < size) and (y < size) and (x >= 0) and (y >= 0))
      {
        _water[x + size * y] = value;
      }
    }

    //! \return moisture if x and y are inside the map, -1 otherwise
    int16_t moisture(int32_t x, int32_t y)
    {
      if ((x < size) and (y < size) and (x >= 0) and (y >= 0))
      {
        return _moisture[x + size * y];
      }
      else
      {
        return -1;
      }
    }

    //! Set moisture if x and y are inside the map
    void moisture(int32_t x, int32_t y, uint8_t value)
    {
      if ((x < size) and (y < size) and (x >= 0) and (y >= 0))
      {
        _moisture[x + size * y] = value;
      }
    }

    void lowest_neighbors (uint16_t & x, uint16_t & y)
    {
      uint16_t center_x = x;
      uint16_t center_y = y;

      int32_t height_min = -1;

      for (int8_t dx = -1 ; dx <= 1 ; dx++)
      {
        for (int8_t dy = -1 ; dy <= 1 ; dy++)
        {
          if ((dx != 0) and (dy != 0))
          {
            if (height_min == -1)
            {
              height_min = height(center_x + dx, center_y + dy);
              x = center_x + dx;
              y = center_y + dy;
            }
            else if ((height(center_x + dx, center_y + dy) != -1) && (height(center_x + dx, center_y + dy) < height_min))
            {
              height_min = height(center_x + dx, center_y + dy);
              x = center_x + dx;
              y = center_y + dy;
            }
          }
        }
      }
    }
      
    uint16_t * _height;  //Height of each pixel of the map
    uint8_t * _water;  //Water power of each pixel, if not null, the pixel is river or lac (ocean is a completly different concept)
    uint8_t * _moisture;  //Moisture of each pixel. 255 = ocean, river, lac,... 0 = desert.
    uint16_t size;    
};

//---------------------------------------------------------------//
//                           Main loop                           //
//---------------------------------------------------------------//

int main ()
{    
  setbuf(stdout, NULL);
    
  // Initialize the random number generator with the seed provided by the configuration
  srand(Config::get().seed);
  
  // Build the map
  Map map;
  
  if (Config::get().generate_topographic_map)
  {
    // Generate the color picker that is used to generate the topographic map
    Topographic_color_picker topographic_color_picker(0, 65535);

    // Save the topographic map
    map.save(&topographic_color_picker, "topographic");
  }
/* TODO
  if (Config::get().generate_biome_map)
  {
    // Generate the color picker that is used to generate the biome map
    Biome_color_picker biome_color_picker();

    // Save the topographic map
    map.save(&biome_color_picker, std::string("biome"));
  }*/
}
