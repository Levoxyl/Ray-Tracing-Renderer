#include <vector>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cfloat>
#include <chrono>

struct Vector3 {
    float x, y, z;
    Vector3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
    
    Vector3 operator+(const Vector3& v) const { 
        return Vector3(x + v.x, y + v.y, z + v.z); 
    }
    Vector3 operator-(const Vector3& v) const { 
        return Vector3(x - v.x, y - v.y, z - v.z); 
    }
    Vector3 operator*(float s) const { 
        return Vector3(x * s, y * s, z * s); 
    }
    Vector3 operator/(float s) const { 
        if (fabs(s) < 1e-8) return *this;
        return Vector3(x / s, y / s, z / s); 
    }
    Vector3 operator-() const { 
        return Vector3(-x, -y, -z);
    }
    
    friend std::ostream& operator<<(std::ostream& os, const Vector3& v) {
        os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
        return os;
    }
};

// Vector math operations - defined once
float dot(const Vector3& a, const Vector3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

float length(const Vector3& v) {
    return std::sqrt(dot(v, v));
}

Vector3 normalize(const Vector3& v) {
    float len = length(v);
    if (len > 1e-8) return v / len;
    return v;
}

Vector3 cross(const Vector3& a, const Vector3& b) {
    return Vector3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

Vector3 reflect(const Vector3& incident, const Vector3& normal) {
    return incident - normal * (2 * dot(incident, normal));
}

struct Ray {
    Vector3 origin;
    Vector3 direction;
    
    Vector3 pointAt(float t) const {
        return origin + direction * t;
    }
};

struct Triangle {
    Vector3 v0, v1, v2;
    Vector3 color;
    bool doubleSided;
    
    Triangle(Vector3 a, Vector3 b, Vector3 c, Vector3 col, bool ds = false)
        : v0(a), v1(b), v2(c), color(col), doubleSided(ds) {}
};

struct HitRecord {
    Vector3 position;
    Vector3 normal;
    float distance;
    const Triangle* triangle;
    
    HitRecord() : distance(FLT_MAX), triangle(nullptr) {}
};

const float EPSILON = 1e-5f;
const float PI = 3.14159265358979323846f;

bool intersectTriangle(const Triangle& tri, const Ray& ray, HitRecord& hit) {
    Vector3 edge1 = tri.v1 - tri.v0;
    Vector3 edge2 = tri.v2 - tri.v0;
    Vector3 h = cross(ray.direction, edge2);
    float a = dot(edge1, h);
    
    // Backface culling - only for single-sided triangles
    if (!tri.doubleSided && a < EPSILON && a > -EPSILON)
        return false;
        
    float f = 1.0f / a;
    Vector3 s = ray.origin - tri.v0;
    float u = f * dot(s, h);
    
    if (u < 0.0 || u > 1.0)
        return false;
        
    Vector3 q = cross(s, edge1);
    float v = f * dot(ray.direction, q);
    
    if (v < 0.0 || u + v > 1.0)
        return false;
        
    float t = f * dot(edge2, q);
    if (t > EPSILON && t < hit.distance) {
        hit.distance = t;
        hit.position = ray.pointAt(t);
        
        // Compute normal - flip for backfaces if double-sided
        hit.normal = normalize(cross(edge1, edge2));
        if (tri.doubleSided && dot(hit.normal, ray.direction) > 0) {
            hit.normal = -hit.normal;
        }
        
        hit.triangle = &tri;
        return true;
    }
    
    return false;
}

Vector3 trace(const Ray& ray, const std::vector<Triangle>& triangles, int depth = 0) {
    if (depth > 3) return Vector3(0, 0, 0); // Prevent infinite recursion
    
    HitRecord closestHit;
    for (const auto& tri : triangles) {
        HitRecord hit;
        if (intersectTriangle(tri, ray, hit)) {
            if (hit.distance < closestHit.distance) {
                closestHit = hit;
            }
        }
    }

    if (!closestHit.triangle) 
        return Vector3(0.2f, 0.7f, 0.8f); // bg color

    // Material properties
    Vector3 materialColor = closestHit.triangle->color;
    float ambientStrength = 0.3f;
    Vector3 ambient = materialColor * ambientStrength;

    // Light settings
    Vector3 lightPos(2, 5, 1);
    Vector3 lightDir = normalize(lightPos - closestHit.position);
    Vector3 viewDir = normalize(ray.origin - closestHit.position);
    Vector3 reflectDir = reflect(-lightDir, closestHit.normal);
    
    // Diffuse lighting
    float diff = std::max(0.0f, dot(closestHit.normal, lightDir));
    Vector3 diffuse = materialColor * diff;
    
    // Specular lighting
    float specularStrength = 0.5f;
    float spec = pow(std::max(0.0f, dot(viewDir, reflectDir)), 32);
    Vector3 specular = Vector3(1,1,1) * spec * specularStrength;
    
    // Shadow check
    Ray shadowRay;
    shadowRay.origin = closestHit.position + closestHit.normal * EPSILON;
    shadowRay.direction = lightDir;
    bool inShadow = false;
    
    for (const auto& tri : triangles) {
        HitRecord shadowHit;
        if (intersectTriangle(tri, shadowRay, shadowHit)) {
            if (shadowHit.distance > 0 && shadowHit.distance < length(lightPos - closestHit.position)) {
                inShadow = true;
                break;
            }
        }
    }
    
    // Reflection for shiny surfaces
    Vector3 reflection(0,0,0);
    if (depth < 3 && materialColor.x > 0.7f) {
        Ray reflectRay;
        reflectRay.origin = closestHit.position + closestHit.normal * EPSILON;
        reflectRay.direction = reflect(ray.direction, closestHit.normal);
        reflection = trace(reflectRay, triangles, depth+1) * 0.5f;
    }

    // Combine lighting
    Vector3 result = ambient;
    if (!inShadow) {
        result = result + diffuse + specular;
    }
    
    return result + reflection;
}

Ray computePrimRay(int x, int y, int width, int height, Vector3 cameraPos) {
    float aspect = width / (float)height;
    float scale = tan(60 * 0.5 * PI / 180);
    
    float px = (2 * ((x + 0.5) / width) - 1) * aspect * scale;
    float py = (1 - 2 * ((y + 0.5) / height)) * scale;
    
    Vector3 direction(px, py, -1);
    direction = normalize(direction);
    
    return Ray{cameraPos, direction};
}

std::vector<Triangle> loadOBJ(const std::string& path, Vector3 color, float scale = 1.0f, 
                                 Vector3 offset = Vector3(0,0,0), bool doubleSided = false) {
    std::vector<Triangle> triangles;
    std::vector<Vector3> vertices;
    
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Error opening OBJ file: " << path << "\n";
        return triangles;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string type;
        iss >> type;
        
        if (type == "v") {
            float x, y, z;
            iss >> x >> y >> z;
            vertices.push_back(Vector3(x, y, z));
        }
        else if (type == "f") {
            std::string v[3];
            iss >> v[0] >> v[1] >> v[2];
            
            int indices[3];
            bool valid = true;
            
            for (int i = 0; i < 3; i++) {
                size_t pos = v[i].find('/');
                if (pos != std::string::npos) {
                    v[i] = v[i].substr(0, pos);
                }
                
                try {
                    indices[i] = std::stoi(v[i]) - 1;
                } catch (...) {
                    valid = false;
                    break;
                }
            }
            
            if (valid && indices[0] >= 0 && indices[0] < vertices.size() &&
                         indices[1] >= 0 && indices[1] < vertices.size() &&
                         indices[2] >= 0 && indices[2] < vertices.size()) {
                Vector3 v0 = vertices[indices[0]] * scale + offset;
                Vector3 v1 = vertices[indices[1]] * scale + offset;
                Vector3 v2 = vertices[indices[2]] * scale + offset;
                triangles.push_back(Triangle(v0, v1, v2, color, doubleSided));
            }
        }
    }
    
    std::cerr << "Loaded " << triangles.size() << " triangles from " << path << "\n";
    return triangles;
}

int main() {
    const int width = 800;
    const int height = 600;
    
    std::vector<Vector3> image(width * height);
    std::vector<Triangle> triangles;
    
    // Position camera properly
    Vector3 cameraPos(0, 1.5, 4);
    
    // Load obj
    std::vector<Triangle> cylinder = loadOBJ("Neshto.obj", 
        Vector3(0.8f, 0.5f, 0.2f),  // Bronze color
        1.0f, 
        Vector3(0, 0, -2),
        true); // Double-sided triangles
    
    triangles.insert(triangles.end(), cylinder.begin(), cylinder.end());
    
    // Add floor
    Vector3 floorColor(0.3f, 0.6f, 0.3f);
    triangles.push_back(Triangle(
        Vector3(-5, -1, -5), Vector3(5, -1, -5), Vector3(5, -1, 5), floorColor, true));
    triangles.push_back(Triangle(
        Vector3(-5, -1, -5), Vector3(5, -1, 5), Vector3(-5, -1, 5), floorColor, true));
    
    // Add back wall
    Vector3 wallColor(0.4f, 0.4f, 0.6f);
    triangles.push_back(Triangle(
        Vector3(-5, 5, -5), Vector3(5, 5, -5), Vector3(5, -1, -5), wallColor, true));
    triangles.push_back(Triangle(
        Vector3(-5, 5, -5), Vector3(5, -1, -5), Vector3(-5, -1, -5), wallColor, true));

    // Add directional light indicator
    Vector3 lightPos(2, 5, 1);
    for (int i = 0; i < 3; i++) {
        Vector3 offset(0.1f, 0.1f, 0.1f);
        if (i == 1) offset = Vector3(-0.1f, 0.1f, 0.1f);
        if (i == 2) offset = Vector3(0.1f, -0.1f, 0.1f);
        triangles.push_back(Triangle(
            lightPos, 
            lightPos + Vector3(0.2f, 0, 0) + offset,
            lightPos + Vector3(0, 0.2f, 0) + offset,
            Vector3(1, 1, 0.5f), true));
    }

    std::cerr << "Rendering " << width << "x" << height << " image...\n";
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            Ray ray = computePrimRay(x, y, width, height, cameraPos);
            image[y * width + x] = trace(ray, triangles);
        }
        
        // Show progress
        if (y % 20 == 0) {
            float progress = (y * 100.0f) / height;
            std::cerr << "Progress: " << progress << "%\r";
            std::cerr.flush();
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cerr << "\nRendering took " << duration.count() << " ms\n";

    // Output PPM
    std::ofstream file("output.ppm");
    file << "P3\n" << width << " " << height << "\n255\n";
    for (auto& color : image) {
        int r = std::min(255, std::max(0, (int)(255 * color.x)));
        int g = std::min(255, std::max(0, (int)(255 * color.y)));
        int b = std::min(255, std::max(0, (int)(255 * color.z)));
        file << r << " " << g << " " << b << "\n";
    }
    file.close();

    std::cerr << "Rendering complete! Saved output.ppm\n";
    return 0;
}