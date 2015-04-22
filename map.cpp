#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

class Color_map
{
  public:
    struct Color
    {
      uint8_t red;
      uint8_t green;
      uint8_t blue;
    };
    
    //! offset : 0 altitude (not 0 height)
    //! min : lowest value in the map, use to configure the scale
    //! max : greatest value in the map, use to configure the scale
    Color_map (uint16_t offset = 65535 / 2, uint16_t min = 0, uint16_t max = 65535)
    {    
      for (uint8_t i = 0 ; i < height_negtive_colors_count ; i++)
      {
        Height_to_color height_to_color;
        height_to_color.color = height_negtive_colors[i];
        height_to_color.height_max = offset - i * ((offset - min) / height_negtive_colors_count);
        height_to_color.height_min = offset - (i + 1) * ((offset - min) / height_negtive_colors_count) - 1;
        height_to_colors.push_back(height_to_color);
      }
      
      for (uint8_t i = 0 ; i < height_colors_count ; i++)
      {
        Height_to_color height_to_color;
        height_to_color.color = height_colors[i];
        height_to_color.height_min = offset + i * ((offset - min) / height_colors_count);
        height_to_color.height_max = offset + (i + 1) * ((offset - min) / height_colors_count) - 1;
        height_to_colors.push_back(height_to_color);
      }      
    }

    Color color (uint16_t height)
    {
      for (auto& height_to_color : height_to_colors) 
      {
        if ((height >= height_to_color.height_max) && (height < height_to_color.height_min))
        {
          return height_to_color.color;
        }
      }    
    }
  
  private:
    struct Height_to_color
    {
      uint16_t height_min;
      uint16_t height_max;
      Color color;
    };
    
    static const uint8_t height_colors_count = 19;
    
    static const uint8_t height_negtive_colors_count = 10;
  
    static const Color height_colors[height_colors_count];

    static const Color height_negtive_colors[height_negtive_colors_count];
    
    std::vector<Height_to_color> height_to_colors;
};

const Color_map::Color Color_map::height_colors[19] = {
  {245, 244 ,242},
  {224, 222, 216},
  {202, 195, 184},
  {186, 174, 154},
  {172, 154, 124},
  {170, 135, 83 },
  {185, 152, 90 },
  {195, 167, 107},
  {202, 185, 130},
  {211, 202, 157},
  {222, 214, 163},
  {232, 225, 182},
  {239, 235, 192},
  {225, 228, 181},
  {209, 215, 171},
  {189, 204, 150},
  {168, 198, 143},
  {148, 191, 139},
  {172, 208, 165}
};

const Color_map::Color Color_map::height_negtive_colors[10] = {
  {216, 242, 254},
  {198, 236, 255},
  {185, 227, 255},
  {172, 219, 251},
  {161, 210, 247},
  {150, 201, 240},
  {141, 193, 234},
  {132, 185, 227},
  {121, 178, 222},
  {113, 171, 216}
};

int is_power_of_two (unsigned int x)
{
  return ((x != 0) && !(x & (x - 1)));
}

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

class Map
{
  public:
    Map (uint16_t map_size)
    {
      //Map size must be a power of two + 1
      
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
      for (uint32_t i = 0 ; i < size * size ; i++)
      {
        _height[i] = 0;
      }
    }
    
    ~Map ()
    {
      delete _height;
    }
      
    void save (Color_map & color_map)
    {
      FILE * fp;

      fp = fopen ("height_map.pgm", "w");
      fprintf(fp, "P2\n");
      fprintf(fp, "%d %d\n", size, size);
      fprintf(fp, "%d\n", height_max());

      for (uint32_t x = 0 ; x < size ; x++)
      {
        for (uint32_t y = 0 ; y < size ; y++)
        {
          fprintf(fp, "%d ", height(x, y));
        }
        fprintf(fp, "\n");
      }

      fclose(fp);
    }
    
    //use Diamond-square algorithm to compote the height map
    //http://en.wikipedia.org/wiki/Diamond-square_algorithm
    void compute_height(uint16_t left_top, uint16_t right_top, uint16_t left_bottom, uint16_t right_bottom, float roughness)
    {    
      //Set the initial corners
      height(0, 0, left_top);
      height(0, size-1, right_top);
      height(size - 1, 0, left_bottom);
      height(size - 1, size-1, right_bottom);
      
      //Each step of the algorithme, the map is splitted in smaler squares
      for (uint32_t square_size = size ; square_size > 2 ; square_size =  square_size / 2 + 1)
      {
        printf("square_size = %d\n", square_size);
        
        //For each squares, compute it's center coordinates
        for (uint32_t x = square_size / 2 ; x < size ; x = x + square_size - 1)
        {
          for (uint32_t y = square_size / 2 ; y < size ; y = y + square_size - 1)
          {           
            //Random offset
            uint16_t offset = rand() * roughness * square_size * 2 - roughness * square_size;
            
            //center value equal the mean of the square corners
            height(x, y, ( height(x - square_size / 2, y - square_size / 2)
                         + height(x + square_size / 2, y - square_size / 2)
                         + height(x - square_size / 2, y + square_size / 2)
                         + height(x + square_size / 2, y + square_size / 2)) / 4 + offset);
          }
        }
        
        //For each diamond, compute it's center coordinates
        for (uint32_t x = 0 ; x < size ; x = x + square_size / 2)
        {
          for (uint32_t y = square_size / 2 - x % (square_size - 1) ; y < size ; y = y + square_size - 1)
          {
            //Random offset
            uint16_t offset = rand() * roughness * square_size * 2 - roughness * square_size;
            
            //center value equal the mean of the diamond corners
            int32_t top = -1;
            int32_t right = -1;
            int32_t bottom = -1;
            int32_t left = -1;
            
            if (((x >= 0) && (x < size)) && ((y - square_size / 2 >= 0) && (y - square_size / 2 < size)))
            {
              top = height(x, y - square_size / 2);
            }
            
            if (((x + square_size / 2 >= 0) && (x + square_size / 2 < size)) && ((y >= 0) && (y < size)))
            {
              right = height(x + square_size / 2, y);
            }
            
            if (((x >= 0) && (x < size)) && ((y + square_size / 2 >= 0) && (y + square_size / 2 < size)))
            {
              bottom = height(x, y + square_size / 2);
            }
            
            if (((x - square_size / 2 >= 0) && (x - square_size / 2 < size)) && ((y >= 0) && (y < size)))
            {
              left = height(x - square_size / 2, y);
            }
            
            height(x, y, average(top, right, bottom, left) + offset);
          }
        }
      }  
    }

    uint16_t height_max()
    {
      uint16_t max = 0;
      for (uint32_t i = 0 ; i < size * size ; i++)
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
      for (uint32_t i = 0 ; i < size * size ; i++)
      {
        if (_height[i] < min)
        {
          min = _height[i];
        }
      }
      return min;
    }
  private:        
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
      
    uint16_t * _height; 
    uint16_t size;
    
};

int main ()
{
  //Generation parameters
  uint32_t seed = 2344541;
  uint32_t map_size = 4000;
  uint32_t left_top_corner_height = 65535 * 0.1;
  uint32_t right_top_corner_height = 65535 * 0.1;
  uint32_t left_bottom_corner_height = 65535 * 0.5;
  uint32_t right_bottom_corner_height = 65535 * 0.5;
  float roughness = 0.5;
  uint32_t ocean_height = 65535 * 0.3;
    
  srand(seed);
  
  Map map(map_size);
  
  map.compute_height(left_top_corner_height, right_top_corner_height, left_bottom_corner_height, right_bottom_corner_height, 0.5);
      
  Color_map color_map;
  //Color_map color_map(ocean_height, map.height_min(), map.height_max());
  
  map.save(color_map);
}
