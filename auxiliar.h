#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_FORCE_RADIANS

#define debug(x) do { std::cerr << #x << ": " << x << "\n"; } while (0)

const float INF = std::numeric_limits<float>::max();
const float PI = std::acos(-1.0f);
const float EPS = 1e-8f;

inline float deg2rad(float deg){ 
    return deg * PI / 180;
}

// Esta función hace que v siempre caiga en el intervalo [lo, hi].
inline float clamp(float lo, float hi, float v) {
    return std::max(lo, std::min(hi, v));
}

std::ostream& operator<< (std::ostream& os, const glm::vec3& v) {
    return os << '[' << v.x << ' ' << v.y << ' ' << v.z << ']';
}

struct Rayo {
    glm::vec3 orig;
    glm::vec3 dir;
    Rayo(const glm::vec3& orig, const glm::vec3& dir): orig(orig), dir(dir) {}
};

struct Triangulo {
    // Uso referencias a los vértices que se encuentran guardados en la malla
    glm::vec3& v0;
    glm::vec3& v1;
    glm::vec3& v2;

    glm::vec3 N; // vector normal (normalizado)
    Triangulo(glm::vec3& v0, glm::vec3& v1, glm::vec3& v2): v0(v0), v1(v1), v2(v2) {
        glm::vec3 v0v1 = v1 - v0;
        glm::vec3 v0v2 = v2 - v0;
        N = glm::normalize(glm::cross(v0v1, v0v2));
        //debug(N);
    }

    bool intersect(const Rayo& rayo, float& t) const {
        // t: parámetro en la ecuación paramétrica del rayo que indica el punto en que se encuentra la intersección.
        // t también es la distancia desde el origen hasta el punto de intersección.

        // 1. Calcular la normal del plano
        // Esto se hizo en el constructor.

        // 2. Calcular la intersección entre el rayo y el plano del triángulo (punto P)
        float N_dot_R = glm::dot(N, rayo.dir);
        
        //debug(N_dot_R);
        // Si la dirección del rayo y la normal del plano son perpendiculares
        if (std::fabs(N_dot_R) < EPS) {
            // Entonces el rayo y el plano son paralelos y no se intersectan.
            return false;
        }

        t = glm::dot(N, v0 - rayo.orig) / N_dot_R;
        //debug(t);
        // 2.1. Si el plano está detrás del rayo, retornar falso.
        if (t < 0) return false;
        
        // 2.2. Calcular P
        glm::vec3 P = rayo.orig + t * rayo.dir;

        // 3. Ver si el punto P está dentro del triángulo (inside-outside test)
        glm::vec3 C;
        
        // Arista 0: del vértice 0 al vértice 1
        glm::vec3 edge0 = v1 - v0;
        glm::vec3 v0p = P - v0;
        C = glm::cross(edge0, v0p);
        if (glm::dot(N, C) < 0) return false;
        
        // Arista 1: del vértice 1 al vértice 2
        glm::vec3 edge1 = v2 - v1;
        glm::vec3 v1p = P - v1;
        C = glm::cross(edge1, v1p);
        if (glm::dot(N, C) < 0) return false;
        
        // Arista 2: del vértice 2 al vértice 0
        glm::vec3 edge2 = v0 - v2;
        glm::vec3 v2p = P - v2;
        C = glm::cross(edge2, v2p);
        if (glm::dot(N, C) < 0) return false;
        
        return true;
    }
};

struct Mesh {
    int num_vertices;
    int num_triangulos;
    std::vector<glm::vec3> vertices;
    std::vector<Triangulo> triangulos;
    glm::vec3 centro;
    glm::mat4 matriz_transformacion; // object to world matrix

    float scale;
    glm::vec3 vec_maxz_malla; // (0, 0, maxz)

    Mesh(const std::string& filename, const glm::mat4& matriz = glm::mat4(1.0f)): matriz_transformacion(matriz) {
        std::ifstream ifs(filename);
        if (!ifs) {
            std::cerr << "No se encontró el archivo.\n";
		    std::exit(1);
        }
        std::string cad;
        ifs >> cad;
        if (cad != "OFF") {
            std::cerr << "Error de formato.\n";
            std::exit(1);
        }
        int num_aristas; // no interesa
        ifs >> num_vertices >> num_triangulos >> num_aristas;
        vertices.reserve(num_vertices);

        for (int i = 0; i < num_vertices; i++) {
            ifs >> vertices[i].x >> vertices[i].y >> vertices[i].z;
            centro.x += vertices[i].x;
            centro.y += vertices[i].y;
            centro.z += vertices[i].z;
        }

        for (int i = 0; i < num_triangulos; i++) {
            int nv; // no interesa
            int ind_v0, ind_v1, ind_v2;
            ifs >> nv >> ind_v0 >> ind_v1 >> ind_v2;
            triangulos.push_back(Triangulo(vertices[ind_v0], vertices[ind_v1], vertices[ind_v2]));
        }
        centro /= num_vertices;

        float maxx = -INF, maxy = -INF, maxz = -INF;
        float minx = INF, miny = INF, minz = INF;

        for (int i = 0; i < num_vertices; i++) {
            minx = std::min(minx, vertices[i].x);
            maxx = std::max(maxx, vertices[i].x);
            miny = std::min(miny, vertices[i].y);
            maxy = std::max(maxy, vertices[i].y);
            minz = std::min(minz, vertices[i].z);
            maxz = std::max(maxz, vertices[i].z);
        }
        vec_maxz_malla = glm::vec3(0.0f, 0.0f, maxz);

        float diag = std::sqrt((maxx - minx)*(maxx - minx) + (maxy - miny)*(maxy - miny) + (maxz - minz)*(maxz - minz));
        scale = 2.0f / diag;
    }

    bool intersect(const Rayo& rayo, float& t_mas_cercano, int& indice_triangulo_mas_cercano) const {
        bool hay_interseccion = false;
        t_mas_cercano = INF;
        for (int i = 0; i < num_triangulos; i++) {
            float t = INF;
            if (triangulos[i].intersect(rayo, t) && t < t_mas_cercano) {
                t_mas_cercano = t;
                hay_interseccion = true;
                indice_triangulo_mas_cercano = i;
            }
        }
        return hay_interseccion;
    }

    void transformar() {
        // Fuerza la aplicación de la matriz de transformación a todos los vértices de la malla.
        // Debido a que los triángulos guardan referencias de los vértices, sus vértices se actualizarán automáticamente.

        // Mientras tanto, actualizar el maxz_malla
        for (int i = 0; i < num_vertices; i++) {
            vertices[i] = glm::vec3(matriz_transformacion * glm::vec4(vertices[i], 1.0f));
        }
        // Transformar también el centro:
        centro = glm::vec3(matriz_transformacion * glm::vec4(centro, 1.0f));
        vec_maxz_malla = glm::vec3(matriz_transformacion * glm::vec4(vec_maxz_malla, 1.0f));
        // Transformar también las normales de los triángulos:
        glm::mat3 mat_inv_transp = glm::transpose(glm::inverse(glm::mat3(matriz_transformacion)));
        for (int i = 0; i < num_triangulos; i++) {
            triangulos[i].N = glm::normalize(mat_inv_transp * triangulos[i].N);
        }
    }

};
