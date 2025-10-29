#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <cmath>
#include <algorithm>

struct Point3D{
	float x, y, z;
};

struct Point2d{
	int x, y;
	float depth;
};

const int WIDTH = 80;
const int HEIGHT = 40;

const float CUBE_SIZE = 20.0;
const float CAM_DIST = 3.0;
const float FOV = 90.0;

const char SHADES[] = ".:-=+*#%@";

std::vector<Point3D> cube_vertices =
{
		    {-1,-1,-1}, {1,-1,-1}, {1,1,-1}, {-1,1,-1},
			{-1,-1,1}, {1,-1,1}, {1,1,1}, {-1,1,1}
};

// Cube edges as pairs of vertex indices
 std::vector<std::pair<int, int>> cube_edges =
 {
     {0,1}, {1,2}, {2,3}, {3,0}, // bottom face
     {4,5}, {5,6}, {6,7}, {7,4}, // top face
     {0,4}, {1,5}, {2,6}, {3,7}  // vertical edges
 };

Point2d project(const Point3D& p){
	
	float aspect = static_cast<float>(WIDTH)/HEIGHT;

	float fovScale = 1.0f/tan(0.5f * FOV * 3.14128/180);

	float z = p.z + CAM_DIST;

	float scale = fovScale / z* CUBE_SIZE;

	return
	{
		static_cast<int>(WIDTH / 2 + p.x * scale * aspect), // X: center + scaled position
		static_cast<int>(HEIGHT / 2 - p.y * scale),         // Y: center - scaled position (Y axis inverted)
		z                                                   // Store depth for shading
	};

}

void rotateX(Point3D& p, float a)
{
	float y = p.y * cos(a) - p.z * sin(a);
	float z = p.y * sin(a) + p.z * cos(a);
		p.y = y;
		p.z = z;
}

void rotateY(Point3D& p, float a)
{
   float x = p.x * cos(a) + p.z * sin(a);
   float z = -p.x * sin(a) + p.z * cos(a);
   p.x = x;
   p.z = z;
         
}

void drawLine(std::vector<std::vector<char>> &screen, const Point2d& p1, const Point2d& p2){
	
	float avgDepth = (p1.depth + p2.depth)/2.0;
	int shadeIdx = static_cast<int>(avgDepth - CAM_DIST*2)%sizeof(SHADES);
	shadeIdx = std::clamp(shadeIdx, 0, static_cast<int>(sizeof(SHADES) - 2));
	char c = SHADES[shadeIdx];	

	int dx = abs(p2.x - p1.x), dy = -abs(p2.y - p1.y);
	int sx = p1.x < p2.x ? 1 : -1, sy = p1.y < p2.y ? 1 : -1;
	int err = dx + dy, e2;
	int x = p1.x, y = p1.y;
	

	while(true){
				if(x > 0 && x < WIDTH && y > 0 && y < HEIGHT){
					if(screen[y][x] == ' ' || SHADES[shadeIdx] > screen[y][x]) {
						screen[y][x] = c;
					}
				}
			

			if(x == p2.x && y == p2.y) break;


			e2 = err * 2;
			if(e2 >= dy){
				err += dy;
				x += sx;
			}

			if(e2 <= dx){
				err += dx;
				y += sy;
			}

	}

}

int main(){
	
	//optimisers
	std::ios_base::sync_with_stdio(false);
	std::cin.tie(NULL);

	std::cout << "\033[?25l"; //hide the cursor

	std::string buffer;
	buffer.reserve((WIDTH + 1)*HEIGHT * 10);
	
	float angleX = 0, angleY = 0;
	
	while(true){
		
		buffer.clear();
		buffer += "\033[H"; // so we don't overlap courtsey from the university of waterloo

		std::vector<std::vector<char>> screen(HEIGHT, std::vector<char>(WIDTH, ' '));

		std::vector<Point3D> rotated = cube_vertices;

		for(auto &v : rotated){
			rotateX(v, angleX);
			rotateY(v, angleY);
		}

		std::vector<Point2d> projected;

		for(const auto &v : rotated){
			projected.push_back(project(v));
		}

		for(const auto &e : cube_edges){
			drawLine(screen, projected[e.first], projected[e.second]);
		}	

		for(const auto &row :  screen){
			buffer.append(row.begin(), row.end());
			buffer += '\n';
		}
		std::cout << buffer << std::flush;

		angleX += 0.02;	
		angleY += 0.05;	
		std::cout << "\033[31m";
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
		std::cout << "\033[2J"; // clear screen
	}
	
	std::cout << "\033[?25h";

	return 0;
}
