#pragma once

#include "linalg/vector.h"

#include <stdint.h>
#include <stdio.h>

/**
 * Material definition from an OBJ file
 */
struct mcc_obj_material
{
    /* Material name (referenced by usemtl in the OBJ file) */
    const char *r_name;
    
    /* Ambient lighting color (Ka) */
    mcc_vec3f ambient_ligting;
    
    /* Diffuse color (Kd) */
    mcc_vec3f diffuse_color;
    
    /* Specular color (Ks) */
    mcc_vec3f specular_color;
    
    /* Specular exponent (Ns) */
    float specular_exponent;
    
    /* Dissolved amount (d) - 1.0 is fully opaque */
    float dissolved;
    
    /* Transmission filter color (Tf) */
    mcc_vec3f transmission_filter_color;
    
    /* Index of refraction (Ni) */
    float refraction_index;

    /* Path to diffuse texture map file (map_Kd) */
    char *o_diffuse_map;

    /* Illumination model (illum) */
    int illum;
};

/**
 * Group of vertices using the same material
 */
struct mcc_obj_vertex_group
{
    /* Indices into the positions, normals, and texture coordinates arrays */
    uint32_t *indices;
    
    /* Number of indices */
    size_t indices_size;
    
    /**
     * Index into the materials array
     * If no material is specified this is set to ~0
     */
    size_t material_index;
};

/**
 * A 3D object from an OBJ file (specified by 'o' or 'g' in the file)
 */
struct mcc_obj_object
{
    /* Object name */
    const char *o_name;

    /* Vertex positions */
    mcc_vec4f *o_positions;
    size_t positions_size;
    
    /* Texture coordinates */
    mcc_vec3f *o_texture_coordinates;
    size_t texture_coordinates_size;
    
    /* Vertex normals */
    mcc_vec3f *o_normals;
    size_t normals_size;
    
    /* Vertex colors (if provided) */
    mcc_vec3f *o_colors;
    size_t colors_size;

    /* Vertex groups (faces grouped by material) */
    struct mcc_obj_vertex_group *o_vertex_groups;
    size_t vertex_groups_size;
};

/**
 * Loaded OBJ file data
 */
struct mcc_obj_loaded_obj
{
    /* Objects in the file */
    struct mcc_obj_object *o_objects;
    size_t objects_size;
    
    /* Materials defined in the MTL file */
    struct mcc_obj_material *o_materials;
    size_t materials_size;
};

/**
 * Loads an OBJ file from a path
 * 
 * @param path Path to the OBJ file
 * @return Loaded OBJ data or NULL if loading failed
 */
struct mcc_obj_loaded_obj *mcc_obj_load_path(const char *path);

/**
 * Loads an OBJ file from an already open file
 * 
 * @param file Open file handle to read from
 * @param base_path Base path for resolving relative paths (e.g., to MTL files)
 * @return Loaded OBJ data or NULL if loading failed
 */
struct mcc_obj_loaded_obj *mcc_obj_load(FILE *file, const char *base_path);

/**
 * Frees memory used by a loaded OBJ file
 * 
 * @param obj The loaded OBJ data to free
 */
void mcc_obj_loaded_obj_free(struct mcc_obj_loaded_obj *obj);

/**
 * Loads an MTL (material) file
 * 
 * @param path Path to the MTL file
 * @param out_materials_count Pointer to store the number of loaded materials
 * @return Array of loaded materials or NULL if loading failed
 */
struct mcc_obj_material *mcc_obj_load_mtl(const char *path, size_t *out_materials_count);
