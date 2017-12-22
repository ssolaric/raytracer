// Para compilar: g++ raytracer.cpp -o raytracer.out -I"./glm-0.9.7.3/glm" -O3

#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <fstream>
#include <vector>
#include <iostream>
#include <cassert>
#include <limits>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_FORCE_RADIANS

#include "auxiliar.h"

void guardar_imagen(const std::string& filename, glm::vec3* imagen, int width, int height) {
    std::ofstream ofs(filename, std::ios::out | std::ios::binary);
    ofs << "P6\n" << width << " " << height << "\n255\n";
    for (int i = 0; i < height * width; i++) {
        unsigned char r = (unsigned char) (255 * clamp(0.0f, 1.0f, imagen[i].x));
        unsigned char g = (unsigned char) (255 * clamp(0.0f, 1.0f, imagen[i].y));
        unsigned char b = (unsigned char) (255 * clamp(0.0f, 1.0f, imagen[i].z));
        ofs << r << g << b;
    }
}

glm::vec3 facing_ratio(const Rayo& rayo, const Triangulo& triangulo) {
    return glm::vec3(std::max(0.0f, glm::dot(triangulo.N, -rayo.dir)));
}

glm::vec3 lanzar_rayo(const Rayo& rayo, const Mesh& mesh) {
    glm::vec3 color_fondo(1.0, 0.0, 0.0); // rojo
    float t_mas_cercano;
    int indice_triangulo_mas_cercano;
    if (mesh.intersect(rayo, t_mas_cercano, indice_triangulo_mas_cercano)) {
        const Triangulo& triangulo_mas_cercano = mesh.triangulos[indice_triangulo_mas_cercano];
        return facing_ratio(rayo, triangulo_mas_cercano) * glm::vec3(0.0f, 1.0f, 0.0f); // sombreado verde
    }
    else {
        return color_fondo;
    }
}

void prueba_malla(const std::string& nombre_archivo) {
    Mesh malla(nombre_archivo + ".off");
    
    glm::mat4 mandar_al_centro = 
        glm::scale(glm::mat4(1.0f), glm::vec3(malla.scale)) *
        glm::translate(glm::mat4(1.0f), -malla.centro);
    
    glm::mat4 transformaciones_centro = 
        glm::mat4(1.0f) *
        glm::rotate(glm::mat4(1.0f), glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f)) *        
        glm::rotate(glm::mat4(1.0f), glm::radians(-30.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    float d = 0.8f; // distancia deseada entre el plano de visión y la malla

    malla.matriz_transformacion = 
        transformaciones_centro *
        mandar_al_centro;

    malla.transformar();

    glm::mat4 mandar_detras_del_plano = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -malla.vec_maxz_malla.z - 1 - d));
    malla.matriz_transformacion = 
        mandar_detras_del_plano;
    malla.transformar();
    
    int width = 640;
    int height = 480;
    
    glm::vec3* imagen = new glm::vec3[width * height];
    glm::vec3* pixel = imagen;
    
    float fov = 90.0f;
    float scale = std::tan(deg2rad(fov * 0.5f));
    float aspect_ratio = width / (float) height;
    
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++, pixel++) {
            // transformar punto de la imagen en memoria a punto de la imagen en pantalla
            float x1 = (2 * (i + 0.25) / (float)width - 1) * aspect_ratio * scale;
            float y1 = (1 - 2 * (j + 0.25) / (float)height) * scale;
            float x2 = (2 * (i + 0.75) / (float)width - 1) * aspect_ratio * scale;
            float y2 = (1 - 2 * (j + 0.25) / (float)height) * scale;
            float x3 = (2 * (i + 0.25) / (float)width - 1) * aspect_ratio * scale;
            float y3 = (1 - 2 * (j + 0.75) / (float)height) * scale;
            float x4 = (2 * (i + 0.75) / (float)width - 1) * aspect_ratio * scale;
            float y4 = (1 - 2 * (j + 0.75) / (float)height) * scale;

            glm::vec3 dir1(x1, y1, -1);
            glm::vec3 dir2(x2, y2, -1);
            glm::vec3 dir3(x3, y3, -1);
            glm::vec3 dir4(x4, y4, -1);
            Rayo rayo1(glm::vec3(0), glm::normalize(dir1));
            Rayo rayo2(glm::vec3(0), glm::normalize(dir2));
            Rayo rayo3(glm::vec3(0), glm::normalize(dir3));
            Rayo rayo4(glm::vec3(0), glm::normalize(dir4));
            glm::vec3 subpixel1 = lanzar_rayo(rayo1, malla);
            glm::vec3 subpixel2 = lanzar_rayo(rayo2, malla);
            glm::vec3 subpixel3 = lanzar_rayo(rayo3, malla);
            glm::vec3 subpixel4 = lanzar_rayo(rayo4, malla);
            *pixel = subpixel1 + subpixel2 + subpixel3 + subpixel4;
            *pixel /= 4;
        }
    }
    guardar_imagen(nombre_archivo + ".ppm", imagen, width, height);
    delete[] imagen;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Uso: ./raytracer.out <archivo OFF>\n";
        std::exit(1);
    }
    std::string nombre_archivo(argv[1]);
    auto ind = nombre_archivo.find(".off");
    if (ind == std::string::npos) {
        std::cout << "El archivo no es de formato OFF.\n";
        std::exit(1);
    }

    nombre_archivo = nombre_archivo.substr(0, ind);

    prueba_malla(nombre_archivo);
    return 0;
}
