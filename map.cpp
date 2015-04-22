#include <cstdint>
#include <cstdio>
#include <string>

#define MAP_SIZE 40000

//http://www.exploringbinary.com/ten-ways-to-check-if-an-integer-is-a-power-of-two-in-c/
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
  public :
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
      
    void save ()
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
    //http://www.playfuljs.com/realistic-terrain-in-130-lines/
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
            //printf("\tsquare center %d;%d\n", x, y);
            //printf("\t\tsquare left_top %d;%d\n", x - square_size / 2, y - square_size / 2);
            //printf("\t\tsquare right_top %d;%d\n", x + square_size / 2, y - square_size / 2);
            //printf("\t\tsquare left_bottom %d;%d\n", x - square_size / 2, y + square_size / 2);
            //printf("\t\tsquare right_bottom %d;%d\n", x + square_size / 2, y + square_size / 2);
            
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
            //printf("\tdiamond center %d;%d\n", x, y);
            //printf("\t\tdiamond top %d;%d\n", x, y - square_size / 2);
            //printf("\t\tdiamond right %d;%d\n", x + square_size / 2, y);
            //printf("\t\tdiamond bottom %d;%d\n", x, y + square_size / 2);
            //printf("\t\tdiamond left %d;%d\n", x - square_size / 2, y);
            
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

  private :
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
  uint32_t map_size = MAP_SIZE;
  
  srand(2344541);
  
  Map map(map_size);
  
  map.compute_height(65535 * 0.5, 65535 * 0.5, 65535 * 0.5, 65535 * 0.1, 0.5);
  
  map.save();
}
