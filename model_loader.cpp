#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <thread>
#include <algorithm>
#include <string>
#include <set>
#include <fstream>
#include <sstream>

#define NOMINMAX
#include <windows.h>

struct Point3D {
	float x, y, z;
};

struct Point2d {
	int x, y;
	float depth;
};

const int WIDTH = 80;
const int HEIGHT = 40;

const float CUBE_SIZE = 20.0;
const float CAM_DIST = 3.0;
const float FOV = 90.0;

const char SHADES[] = ".:-=+*#%@";

//model data
std::vector<Point3D> model_vertices;
std::vector<std::pair<int, int>> model_edges;


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

Point2d project(const Point3D& p) {

	float aspect = static_cast<float>(WIDTH) / HEIGHT;

	float fovScale = 1.0f / tan(0.5f * FOV * 3.14159f / 180.0f);

	float z = p.z + CAM_DIST;

	float scale = fovScale / z * CUBE_SIZE;

	return
	{
		static_cast<int>(WIDTH / 2 + p.x * scale * aspect),
		static_cast<int>(HEIGHT / 2 - p.y * scale),
		z
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

void drawLine(std::vector<std::vector<char>>& screen, const Point2d& p1, const Point2d& p2) {

	float avgDepth = (p1.depth + p2.depth) / 2.0f;
	int shadeIdx = static_cast<int>((avgDepth - CAM_DIST) * 2.0f);
	shadeIdx = std::clamp(shadeIdx, 0, static_cast<int>(sizeof(SHADES) - 2));
	char c = SHADES[shadeIdx];

	int dx = abs(p2.x - p1.x), dy = -abs(p2.y - p1.y);
	int sx = p1.x < p2.x ? 1 : -1, sy = p1.y < p2.y ? 1 : -1;
	int err = dx + dy, e2;
	int x = p1.x, y = p1.y;

	while (true) {
		if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
			if (screen[y][x] == ' ' || c > screen[y][x]) {
				screen[y][x] = c;
			}
		}

		if (x == p2.x && y == p2.y) break;

		e2 = err * 2;
		if (e2 >= dy) {
			err += dy;
			x += sx;
		}

		if (e2 <= dx) {
			err += dx;
			y += sy;
		}
	}
}

bool loadObj(const std::string& filename) {
	std::ifstream file(filename);
	if (!file.is_open()) {
		std::cerr << "Failed to open OBJ file: " << filename << std::endl;
		return false;
	}

	model_vertices.clear();
	model_edges.clear();

	std::string line;
	std::vector<std::vector<int>> faces;

	while (std::getline(file, line))
	{
		if (line.empty() || line[0] == '#') continue;

		std::istringstream iss(line);
		std::string type;
		iss >> type;

		if (type == "v") {
			float x, y, z;
			iss >> x >> y >> z;
			model_vertices.push_back({ x, y, z });
		}
		else if (type == "f") {
			std::vector<int> faceIndices;
			std::string vertexData;
			while (iss >> vertexData) {
				size_t slashPos = vertexData.find('/');
				int vertexIndex = std::stoi(slashPos == std::string::npos ? vertexData : vertexData.substr(0, slashPos)) - 1;
				if (vertexIndex >= 0) {
					faceIndices.push_back(vertexIndex);
				}
				else {
					faceIndices.push_back(static_cast<int>(model_vertices.size()) + vertexIndex + 1);
				}
			}
			if (faceIndices.size() >= 3) {
				faces.push_back(faceIndices);
			}
		}
		else if (type == "l") {
			std::vector<int> line_vertices;
			std::string vertexData;

			while (iss >> vertexData) {
				size_t slashPos = vertexData.find('/');
				int vertexIndex = std::stoi(slashPos == std::string::npos ? vertexData : vertexData.substr(0, slashPos)) - 1;
				if (vertexIndex >= 0) {
					line_vertices.push_back(vertexIndex);
				}
				else {
					line_vertices.push_back(static_cast<int>(model_vertices.size()) + vertexIndex + 1);
				}
			}

			for (size_t i = 0; i < line_vertices.size() - 1; i++) {
				model_edges.push_back({ line_vertices[i], line_vertices[i + 1] });
			}
		}
	}

	file.close();

	if (model_vertices.empty()) {
		std::cerr << "No vertices found in OBJ file: " << filename << std::endl;
		return false;
	}

	if (model_edges.empty()) {
		std::set<std::pair<int, int>> edgeSet;

		for (const auto& face : faces) {
			for (size_t i = 0; i < face.size(); i++) {
				int v1 = face[i];
				int v2 = face[(i + 1) % face.size()];
				if (v1 > v2) std::swap(v1, v2);
				edgeSet.insert({ v1, v2 });
			}
		}
		model_edges.assign(edgeSet.begin(), edgeSet.end());
	}

	if (!model_vertices.empty()) {
		float min_x = model_vertices[0].x, max_x = model_vertices[0].x;
		float min_y = model_vertices[0].y, max_y = model_vertices[0].y;
		float min_z = model_vertices[0].z, max_z = model_vertices[0].z;

		for (const auto& v : model_vertices)
		{
			min_x = std::min(min_x, v.x); max_x = std::max(max_x, v.x);
			min_y = std::min(min_y, v.y); max_y = std::max(max_y, v.y);
			min_z = std::min(min_z, v.z); max_z = std::max(max_z, v.z);
		}

		float center_x = (min_x + max_x) / 2.0f;
		float center_y = (min_y + max_y) / 2.0f;
		float center_z = (min_z + max_z) / 2.0f;

		float max_size = std::max({ max_x - min_x, max_y - min_y, max_z - min_z });
		float scale = max_size > 0 ? 2.0f / max_size : 1.0f;
		scale *= 2;
		for (auto& v : model_vertices) {
			v.x = (v.x - center_x) * scale;
			v.y = (v.y - center_y) * scale;
			v.z = (v.z - center_z) * scale;
		}

		std::cout << "Loaded OBJ: " << filename << " with " << model_vertices.size()
			<< " vertices and " << model_edges.size() << " edges.\n";
	}

	return true;
}

void setSquareFont()
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_FONT_INFOEX fontInfo = { sizeof(fontInfo) };
	fontInfo.dwFontSize.X = 8;
	fontInfo.dwFontSize.Y = 8;
	wcscpy_s(fontInfo.FaceName, L"Terminal");
	SetCurrentConsoleFontEx(hConsole, FALSE, &fontInfo);
}

int main(int argc, char* argv[])
{
	setSquareFont();
	std::ios_base::sync_with_stdio(false);
	std::cin.tie(NULL);

	std::cout << "\033[?25l"; //hide the cursor

	std::string buffer;
	buffer.reserve((WIDTH + 1) * HEIGHT * 10);

	float angleX = 0, angleY = 0;

	model_vertices = cube_vertices;
	model_edges = cube_edges;

	if (argc > 1) {
		if (!loadObj(argv[1])) {
			std::cerr << "Failed to load model. Using default cube.\n";
		}
	}

	while (true) {
		buffer.clear();
		buffer += "\033[H";

		std::vector<std::vector<char>> screen(HEIGHT, std::vector<char>(WIDTH, ' '));

		std::vector<Point3D> rotated = model_vertices;

		for (auto& v : rotated) {
			rotateX(v, angleX);
			rotateY(v, angleY);
		}

		std::vector<Point2d> projected;
		for (const auto& v : rotated) {
			projected.push_back(project(v));
		}

		for (const auto& e : model_edges) {
			drawLine(screen, projected[e.first], projected[e.second]);
		}

		for (const auto& row : screen) {
			buffer.append(row.begin(), row.end());
			buffer += '\n';
		}

		std::cout << buffer << std::flush;

		angleX += 0.02f;
		angleY += 0.05f;

		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	std::cout << "\033[?25h";

	return 0;
}