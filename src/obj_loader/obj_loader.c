#include "obj_loader.h"
#include "../hash_map/hash_map.h"
#include "../safe_cast.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>

const char *DEFAULT_MATERIAL_NAME = "unnamed";

#define MCC_OBJ_ERROR(format, ...) \
    do { \
        fprintf(stderr, "Error: " format "\n" __VA_OPT__(,) __VA_ARGS__); \
        exit(EXIT_FAILURE); \
    } while (0)

static void mcc_obj_material_init(struct mcc_obj_material *material) {
    if (!material)
        return;
        
    *material = (struct mcc_obj_material) {
        .r_name = DEFAULT_MATERIAL_NAME,
        .ambient_ligting = {{ 1.f, 1.f, 1.f }},
        .diffuse_color = {{ 1.f, 1.f, 1.f }},
        .specular_color = {{ 0.f, 0.f, 0.f }},
        .specular_exponent = 0.f,
        .dissolved = 1.f,
        .transmission_filter_color = {{ 1., 1., 1. }},
        .refraction_index = 1.f,
        .o_diffuse_map = NULL,
        .illum = 0,
    };
}

static void mcc_obj_material_free(struct mcc_obj_material *material) {
    if (material->r_name != DEFAULT_MATERIAL_NAME)
        free((char*)material->r_name);
    free(material->o_diffuse_map);
}

static void mcc_obj_vertex_group_init(struct mcc_obj_vertex_group *group) {
    if (!group)
        return;
        
    *group = (struct mcc_obj_vertex_group) {
        .indices = NULL,
        .indices_size = 0,
        .material_index = SIZE_MAX
    };
}

static bool mcc_obj_object_init(struct mcc_obj_object *obj, const char *name) {
    if (!obj)
        return false;
    
    char *name_copy = NULL;
    if (name) {
        name_copy = strdup(name);
        if (!name_copy) {
            return false;
        }
    }
    
    *obj = (struct mcc_obj_object) {
        .o_name = name_copy,
        .o_positions = NULL,
        .positions_size = 0,
        .o_texture_coordinates = NULL,
        .texture_coordinates_size = 0,
        .o_normals = NULL,
        .normals_size = 0,
        .o_colors = NULL,
        .colors_size = 0,
        .o_vertex_groups = NULL,
        .vertex_groups_size = 0
    };
    
    return true;
}

// Get directory from a filepath
static char *get_directory_path(const char *path) {
    char *path_copy = strdup(path);
    if (!path_copy) {
        MCC_OBJ_ERROR("Memory allocation failed");
    }
    
    char *last_slash = strrchr(path_copy, '/');
    if (last_slash) {
        *(last_slash + 1) = '\0';
        return path_copy;
    } else {
        free(path_copy);
        return strdup("./");
    }
}

// Combine base path with a filename
static char *combine_paths(const char *base_path, const char *filename) {
    size_t base_len = strlen(base_path);
    size_t file_len = strlen(filename);
    char *result = malloc(base_len + file_len + 1);
    
    if (!result) {
        MCC_OBJ_ERROR("Memory allocation failed");
    }
    
    strcpy(result, base_path);
    strcat(result, filename);
    
    return result;
}

struct mcc_obj_material *mcc_obj_load_mtl(const char *path, size_t *out_materials_count) {
    FILE *file = fopen(path, "r");
    if (!file) {
        MCC_OBJ_ERROR("Failed to open MTL file: %s", path);
    }
    
    struct mcc_obj_material *materials = NULL;
    size_t materials_size = 0;
    size_t materials_capacity = 0;
    
    struct mcc_obj_material current_material;
    mcc_obj_material_init(&current_material);
    current_material.r_name = NULL; // Not set yet
    
    char *base_path = get_directory_path(path);
    
    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;
            
        char *token = strtok(line, " \t\n\r");
        if (!token)
            continue;
            
        if (strcmp(token, "newmtl") == 0) {
            // Save previous material if it has a name
            if (current_material.r_name) {
                if (materials_size >= materials_capacity) {
                    materials_capacity = materials_capacity == 0 ? 4 : materials_capacity * 2;
                    struct mcc_obj_material *new_materials = realloc(materials, 
                                                                  materials_capacity * sizeof(struct mcc_obj_material));
                    if (!new_materials) {
                        fclose(file);
                        free(base_path);
                        free(materials);
                        MCC_OBJ_ERROR("Memory allocation failed");
                    }
                    materials = new_materials;
                }
                
                materials[materials_size++] = current_material;
            }
            
            // Start new material
            mcc_obj_material_init(&current_material);
            
            // Get material name
            char *name = strtok(NULL, " \t\n\r");
            if (name) {
                current_material.r_name = strdup(name);
                if (!current_material.r_name) {
                    fclose(file);
                    free(base_path);
                    free(materials);
                    MCC_OBJ_ERROR("Memory allocation failed");
                }
            }
        }
        else if (strcmp(token, "Ka") == 0) {
            float r = strtof(strtok(NULL, " \t\n\r"), NULL);
            float g = strtof(strtok(NULL, " \t\n\r"), NULL);
            float b = strtof(strtok(NULL, " \t\n\r"), NULL);
            current_material.ambient_ligting = (mcc_vec3f){{ r, g, b }};
        }
        else if (strcmp(token, "Kd") == 0) {
            float r = strtof(strtok(NULL, " \t\n\r"), NULL);
            float g = strtof(strtok(NULL, " \t\n\r"), NULL);
            float b = strtof(strtok(NULL, " \t\n\r"), NULL);
            current_material.diffuse_color = (mcc_vec3f){{ r, g, b }};
        }
        else if (strcmp(token, "Ks") == 0) {
            float r = strtof(strtok(NULL, " \t\n\r"), NULL);
            float g = strtof(strtok(NULL, " \t\n\r"), NULL);
            float b = strtof(strtok(NULL, " \t\n\r"), NULL);
            current_material.specular_color = (mcc_vec3f){{ r, g, b }};
        }
        else if (strcmp(token, "Ns") == 0) {
            current_material.specular_exponent = strtof(strtok(NULL, " \t\n\r"), NULL);
        }
        else if (strcmp(token, "d") == 0) {
            current_material.dissolved = strtof(strtok(NULL, " \t\n\r"), NULL);
        }
        else if (strcmp(token, "Tr") == 0) {
            float tr = strtof(strtok(NULL, " \t\n\r"), NULL);
            current_material.dissolved = 1.0f - tr;
        }
        else if (strcmp(token, "Tf") == 0) {
            float r = strtof(strtok(NULL, " \t\n\r"), NULL);
            float g = strtof(strtok(NULL, " \t\n\r"), NULL);
            float b = strtof(strtok(NULL, " \t\n\r"), NULL);
            current_material.transmission_filter_color = (mcc_vec3f){{ r, g, b }};
        }
        else if (strcmp(token, "Ni") == 0) {
            current_material.refraction_index = strtof(strtok(NULL, " \t\n\r"), NULL);
        }
        else if (strcmp(token, "illum") == 0) {
            current_material.illum = atoi(strtok(NULL, " \t\n\r"));
        }
        else if (strcmp(token, "map_Kd") == 0) {
            char *map_file = strtok(NULL, " \t\n\r");
            if (map_file) {
                char *full_path = combine_paths(base_path, map_file);
                current_material.o_diffuse_map = full_path;
            }
        }
    }
    
    // Don't forget the last material
    if (current_material.r_name) {
        if (materials_size >= materials_capacity) {
            materials_capacity = materials_capacity == 0 ? 4 : materials_capacity * 2;
            struct mcc_obj_material *new_materials = realloc(materials, 
                                                          materials_capacity * sizeof(struct mcc_obj_material));
            if (!new_materials) {
                fclose(file);
                free(base_path);
                free(materials);
                MCC_OBJ_ERROR("Memory allocation failed");
            }
            materials = new_materials;
        }
        
        materials[materials_size++] = current_material;
    }
    
    free(base_path);
    fclose(file);
    
    if (out_materials_count)
        *out_materials_count = materials_size;
    
    return materials;
}

struct mcc_obj_loaded_obj *mcc_obj_load_path(const char *path) {
    FILE *file = fopen(path, "r");
    if (!file) {
        MCC_OBJ_ERROR("Failed to open OBJ file: %s", path);
    }
    
    char *base_path = get_directory_path(path);
    struct mcc_obj_loaded_obj *obj = mcc_obj_load(file, base_path);
    
    free(base_path);
    fclose(file);
    
    return obj;
}

/**
 * Hash function for material name lookup
 * Uses Jenkins one-at-a-time hash
 */
static size_t material_hash(const void *key, size_t seed) {
    const char *str = key;
    size_t hash = seed;
    unsigned char c;
    
    while ((c = (unsigned char)*str++)) {
        hash += (size_t)c;
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    
    return hash;
}

/**
 * Wrapper function to free both key and value in the material hash map
 */
static void material_map_entry_free(void *key, void *value) {
    free(key);    // Free the key (material name)
    free(value);  // Free the value (size_t* index pointer)
}

/**
 * Frees memory associated with a hash map storing material indices
 */
static void material_map_free(struct mcc_hmap *map) {
    if (!map)
        return;
    
    // Free all the keys and values
    mcc_hmap_for_all(map, material_map_entry_free);
    
    // Free the hash map itself
    mcc_hmap_destroy(map);
}

/**
 * Adds a material name and index to the material hash map
 */
static void material_map_add(struct mcc_hmap *map, const char *name, size_t index) {
    if (!map || !name)
        return;
    
    // We need to duplicate the name since it's owned by the material
    char *name_copy = strdup(name);
    if (!name_copy) {
        MCC_OBJ_ERROR("Memory allocation failed for material name");
    }
    
    // Store the index as the value (using size_t pointer)
    size_t *index_ptr = malloc(sizeof(size_t));
    if (!index_ptr) {
        free(name_copy);
        MCC_OBJ_ERROR("Memory allocation failed for material index");
    }
    
    *index_ptr = index;
    
    // Add to hash map
    if (!mcc_hmap_add(map, name_copy, index_ptr)) {
        // If adding fails (e.g., duplicate), clean up
        free(name_copy);
        free(index_ptr);
    }
}

/**
 * Finds a material index by name in the hash map
 * Returns SIZE_MAX if not found
 */
static size_t material_map_find(struct mcc_hmap *map, const char *name) {
    if (!map || !name)
        return SIZE_MAX;
    
    struct mcc_hmap_element element = mcc_hmap_find(map, (void*)name);
    if (element.value) {
        return *(size_t*)element.value;
    }
    
    return SIZE_MAX;
}

struct mcc_obj_loaded_obj *mcc_obj_load(FILE *file, const char *base_path) {
    if (!file || !base_path)
        return NULL;
    
    struct mcc_obj_loaded_obj *loaded_obj = malloc(sizeof(*loaded_obj));
    if (!loaded_obj)
        return NULL;
    
    *loaded_obj = (struct mcc_obj_loaded_obj) {
        .o_objects = NULL,
        .objects_size = 0,
        .o_materials = NULL,
        .materials_size = 0
    };
    
    // Material map for name lookup using our hash map
    struct mcc_hmap_create_params params = {
        .capacity = 16,
        .hash_func = material_hash,
        .second_hash_func = NULL,
        .compare_func = NULL
    };
    struct mcc_hmap *material_map = mcc_hmap_create(&params);
    if (!material_map) {
        free(loaded_obj);
        return NULL;
    }
    
    // Temporary buffers for parsing
    char line[1024];
    mcc_vec4f *positions = NULL;
    size_t positions_capacity = 0;
    size_t positions_size = 0;
    
    mcc_vec3f *texture_coords = NULL;
    size_t texture_coords_capacity = 0;
    size_t texture_coords_size = 0;
    
    mcc_vec3f *normals = NULL;
    size_t normals_capacity = 0;
    size_t normals_size = 0;
    
    mcc_vec3f *colors = NULL;
    size_t colors_capacity = 0;
    size_t colors_size = 0;
    
    struct mcc_obj_object *current_object = NULL;
    struct mcc_obj_vertex_group *current_group = NULL;
    
    while (fgets(line, sizeof(line), file)) {
        char *token = strtok(line, " \t\n\r");
        if (!token)
            continue;
        
        if (strcmp(token, "v") == 0) {
            // Parse a series of floating point values for position and color (optional)
            float values[7]; // 3-4 for position, 0-3 for color
            int num_values = 0;
            char *value_str;
            
            while ((value_str = strtok(NULL, " \t\n\r")) != NULL && num_values < 7) {
                values[num_values++] = strtof(value_str, NULL);
            }
            
            if (num_values < 3) {
                MCC_OBJ_ERROR("Vertex position must have at least 3 coordinates");
            }
            
            // Position
            float x = values[0];
            float y = values[1];
            float z = values[2];
            float w = num_values > 3 ? values[3] : 1.0f;
            
            if (positions_size >= positions_capacity) {
                positions_capacity = positions_capacity == 0 ? 16 : positions_capacity * 2;
                mcc_vec4f *new_positions = realloc(positions, positions_capacity * sizeof(mcc_vec4f));
                if (!new_positions) {
                    mcc_obj_loaded_obj_free(loaded_obj);
                    free(positions);
                    free(texture_coords);
                    free(normals);
                    free(colors);
                    material_map_free(material_map);
                    return NULL;
                }
                positions = new_positions;
            }
            
            positions[positions_size++] = (mcc_vec4f){{ x, y, z, w }};
            
            // Color data (optional, r,g,b after position)
            int color_start = (num_values > 3 && values[3] > 1.0f) ? 3 : 4;
            if (num_values >= color_start + 3) {
                if (colors_size >= colors_capacity) {
                    colors_capacity = colors_capacity == 0 ? 16 : colors_capacity * 2;
                    mcc_vec3f *new_colors = realloc(colors, colors_capacity * sizeof(mcc_vec3f));
                    if (!new_colors) {
                        mcc_obj_loaded_obj_free(loaded_obj);
                        free(positions);
                        free(texture_coords);
                        free(normals);
                        free(colors);
                        material_map_free(material_map);
                        return NULL;
                    }
                    colors = new_colors;
                }
                
                colors[colors_size++] = (mcc_vec3f){{ 
                    values[color_start],
                    values[color_start + 1],
                    values[color_start + 2]
                }};
            }
        }
        else if (strcmp(token, "vt") == 0) {
            // Texture coordinate
            float u = strtof(strtok(NULL, " \t\n\r"), NULL);
            float v = 0.0f;
            float w = 0.0f;
            
            char *v_str = strtok(NULL, " \t\n\r");
            if (v_str) {
                v = strtof(v_str, NULL);
                
                char *w_str = strtok(NULL, " \t\n\r");
                if (w_str)
                    w = strtof(w_str, NULL);
            }
            
            if (texture_coords_size >= texture_coords_capacity) {
                texture_coords_capacity = texture_coords_capacity == 0 ? 16 : texture_coords_capacity * 2;
                mcc_vec3f *new_texture_coords = realloc(texture_coords, texture_coords_capacity * sizeof(mcc_vec3f));
                if (!new_texture_coords) {
                    mcc_obj_loaded_obj_free(loaded_obj);
                    free(positions);
                    free(texture_coords);
                    free(normals);
                    free(colors);
                    material_map_free(material_map);
                    return NULL;
                }
                texture_coords = new_texture_coords;
            }
            
            texture_coords[texture_coords_size++] = (mcc_vec3f){{ u, v, w }};
        }
        else if (strcmp(token, "vn") == 0) {
            // Normal vector
            float x = strtof(strtok(NULL, " \t\n\r"), NULL);
            float y = strtof(strtok(NULL, " \t\n\r"), NULL);
            float z = strtof(strtok(NULL, " \t\n\r"), NULL);
            
            if (normals_size >= normals_capacity) {
                normals_capacity = normals_capacity == 0 ? 16 : normals_capacity * 2;
                mcc_vec3f *new_normals = realloc(normals, normals_capacity * sizeof(mcc_vec3f));
                if (!new_normals) {
                    mcc_obj_loaded_obj_free(loaded_obj);
                    free(positions);
                    free(texture_coords);
                    free(normals);
                    if (colors) {
                        free(colors);
                    }
                    material_map_free(material_map);
                    return NULL;
                }
                normals = new_normals;
            }
            
            normals[normals_size++] = (mcc_vec3f){{ x, y, z }};
        }
        else if (strcmp(token, "o") == 0 || strcmp(token, "g") == 0) {
            // Object or group
            char *name = strtok(NULL, " \t\n\r");
            
            if (current_object) {
                // Finalize current object
                if (positions_size > 0) {
                    current_object->o_positions = malloc(positions_size * sizeof(mcc_vec4f));
                    if (!current_object->o_positions) {
                        mcc_obj_loaded_obj_free(loaded_obj);
                        free(positions);
                        if (texture_coords) {
                            free(texture_coords);
                        }
                        if (normals) {
                            free(normals);
                        }
                        free(colors);
                        material_map_free(material_map);
                        return NULL;
                    }
                    memcpy(current_object->o_positions, positions, positions_size * sizeof(mcc_vec4f));
                    current_object->positions_size = positions_size;
                }
                
                if (texture_coords_size > 0) {
                    current_object->o_texture_coordinates = malloc(texture_coords_size * sizeof(mcc_vec3f));
                    if (!current_object->o_texture_coordinates) {
                        mcc_obj_loaded_obj_free(loaded_obj);
                        free(positions);
                        if (texture_coords) {
                            free(texture_coords);
                        }
                        if (normals) {
                            free(normals);
                        }
                        free(colors);
                        material_map_free(material_map);
                        return NULL;
                    }
                    memcpy(current_object->o_texture_coordinates, texture_coords, texture_coords_size * sizeof(mcc_vec3f));
                    current_object->texture_coordinates_size = texture_coords_size;
                }
                
                if (normals_size > 0) {
                    current_object->o_normals = malloc(normals_size * sizeof(mcc_vec3f));
                    if (!current_object->o_normals) {
                        mcc_obj_loaded_obj_free(loaded_obj);
                        free(positions);
                        if (texture_coords) {
                            free(texture_coords);
                        }
                        if (normals) {
                            free(normals);
                        }
                        free(colors);
                        material_map_free(material_map);
                        return NULL;
                    }
                    memcpy(current_object->o_normals, normals, normals_size * sizeof(mcc_vec3f));
                    current_object->normals_size = normals_size;
                }
                
                if (colors_size > 0) {
                    current_object->o_colors = malloc(colors_size * sizeof(mcc_vec3f));
                    if (!current_object->o_colors) {
                        mcc_obj_loaded_obj_free(loaded_obj);
                        free(positions);
                        if (texture_coords) {
                            free(texture_coords);
                        }
                        if (normals) {
                            free(normals);
                        }
                        free(colors);
                        material_map_free(material_map);
                        return NULL;
                    }
                    memcpy(current_object->o_colors, colors, colors_size * sizeof(mcc_vec3f));
                    current_object->colors_size = colors_size;
                }
            }
            
            // Add new object to loaded_obj
            struct mcc_obj_object *new_objects = realloc(loaded_obj->o_objects, 
                                                       (loaded_obj->objects_size + 1) * sizeof(struct mcc_obj_object));
            if (!new_objects) {
                mcc_obj_loaded_obj_free(loaded_obj);
                free(positions);
                if (texture_coords) {
                    free(texture_coords);
                }
                if (normals) {
                    free(normals);
                }
                if (colors) {
                    free(colors);
                }
                material_map_free(material_map);
                return NULL;
            }
            
            loaded_obj->o_objects = new_objects;
            
            // Initialize the object directly in the array
            if (!mcc_obj_object_init(&loaded_obj->o_objects[loaded_obj->objects_size], name)) {
                mcc_obj_loaded_obj_free(loaded_obj);
                free(positions);
                if (texture_coords) {
                    free(texture_coords);
                }
                if (normals) {
                    free(normals);
                }
                if (colors) {
                    free(colors);
                }
                material_map_free(material_map);
                return NULL;
            }
            
            current_object = &loaded_obj->o_objects[loaded_obj->objects_size++];
            current_group = NULL;
        }
        else if (strcmp(token, "f") == 0) {
            // Face definition
            if (!current_object) {
                // Create default object if none exists
                struct mcc_obj_object *new_objects = realloc(loaded_obj->o_objects, 
                                                           (loaded_obj->objects_size + 1) * sizeof(struct mcc_obj_object));
                if (!new_objects) {
                    mcc_obj_loaded_obj_free(loaded_obj);
                    free(positions);
                    free(texture_coords);
                    free(normals);
                    if (colors) {
                        free(colors);
                    }
                    material_map_free(material_map);
                    return NULL;
                }
                
                loaded_obj->o_objects = new_objects;
                
                // Initialize the object directly in the array
                if (!mcc_obj_object_init(&loaded_obj->o_objects[loaded_obj->objects_size], "default")) {
                    mcc_obj_loaded_obj_free(loaded_obj);
                    free(positions);
                    free(texture_coords);
                    free(normals);
                    if (colors) {
                        free(colors);
                    }
                    material_map_free(material_map);
                    return NULL;
                }
                
                current_object = &loaded_obj->o_objects[loaded_obj->objects_size++];
            }
            
            if (!current_group) {
                // Create default group if none exists
                struct mcc_obj_vertex_group *new_groups = realloc(current_object->o_vertex_groups,
                                                               (current_object->vertex_groups_size + 1) * 
                                                               sizeof(struct mcc_obj_vertex_group));
                if (!new_groups) {
                    mcc_obj_loaded_obj_free(loaded_obj);
                    free(positions);
                    free(texture_coords);
                    free(normals);
                    if (colors) {
                        free(colors);
                    }
                    material_map_free(material_map);
                    return NULL;
                }
                
                current_object->o_vertex_groups = new_groups;
                
                // Initialize the group directly in the array
                mcc_obj_vertex_group_init(&current_object->o_vertex_groups[current_object->vertex_groups_size]);
                current_group = &current_object->o_vertex_groups[current_object->vertex_groups_size++];
            }
            
            // Parse face indices
            uint32_t *indices = NULL;
            size_t indices_size = 0;
            size_t indices_capacity = 0;
            size_t face_vertex_count = 0;
            
            char *vertex_str;
            while ((vertex_str = strtok(NULL, " \t\n\r")) != NULL) {
                face_vertex_count++;
                if (face_vertex_count == 4) {
                    MCC_OBJ_ERROR("Obj file has non-triangle faces which isn't supported");
                }
                
                if (indices_size >= indices_capacity) {
                    indices_capacity = indices_capacity == 0 ? 3 : indices_capacity * 2;
                    uint32_t *new_indices = realloc(indices, indices_capacity * sizeof(uint32_t));
                    if (!new_indices) {
                        free(indices);
                        mcc_obj_loaded_obj_free(loaded_obj);
                        free(positions);
                        if (texture_coords) {
                            free(texture_coords);
                        }
                        if (normals) {
                            free(normals);
                        }
                        free(colors);
                        material_map_free(material_map);
                        return NULL;
                    }
                    indices = new_indices;
                }
                
                // Parse vertex index in the form of v/vt/vn or v//vn or v/vt or v
                char *p = vertex_str;
                char *endptr;
                
                unsigned long v_idx_raw = strtoul(p, &endptr, 10);
                if (v_idx_raw == 0) {
                    MCC_OBJ_ERROR("Position index must be at least 1");
                }
                v_idx_raw--;  // OBJ indices are 1-based
                
                uint32_t v_idx = safe_to_u32(v_idx_raw);
                if (v_idx >= positions_size) {
                    MCC_OBJ_ERROR("Position index out of bounds: %u (max: %zu)", 
                                 v_idx + 1, positions_size);
                }
                indices[indices_size++] = v_idx;
                
                // Parse texture coordinate and normal indices
                if (*endptr == '/') {
                    // Parse texture coordinate index
                    p = endptr + 1;
                    if (*p != '/') { // Has texture coordinate
                        unsigned long vt_idx_raw = strtoul(p, &endptr, 10);
                        if (vt_idx_raw > 0) { // Valid index
                            vt_idx_raw--; // OBJ indices are 1-based
                            uint32_t vt_idx = safe_to_u32(vt_idx_raw);
                            if (vt_idx >= texture_coords_size && texture_coords_size > 0) {
                                MCC_OBJ_ERROR("Texture coordinate index out of bounds: %u (max: %zu)",
                                          vt_idx + 1, texture_coords_size);
                            }
                        }
                    } else {
                        endptr = p;
                    }
                    
                    // Parse normal index
                    if (*endptr == '/') {
                        p = endptr + 1;
                        if (*p != '\0') { // Has normal
                            unsigned long vn_idx_raw = strtoul(p, &endptr, 10);
                            if (vn_idx_raw > 0) { // Valid index
                                vn_idx_raw--; // OBJ indices are 1-based
                                uint32_t vn_idx = safe_to_u32(vn_idx_raw);
                                if (vn_idx >= normals_size && normals_size > 0) {
                                    MCC_OBJ_ERROR("Normal index out of bounds: %u (max: %zu)",
                                              vn_idx + 1, normals_size);
                                }
                            }
                        }
                    }
                }
            }
            
            if (face_vertex_count != 3) {
                MCC_OBJ_ERROR("Obj file has non-triangle faces which isn't supported");
            }
            
            // Add indices to current group
            size_t old_size = current_group->indices_size;
            uint32_t *new_group_indices = realloc(current_group->indices, 
                                              (current_group->indices_size + indices_size) * sizeof(uint32_t));
            if (!new_group_indices) {
                free(indices);
                mcc_obj_loaded_obj_free(loaded_obj);
                free(positions);
                if (texture_coords) {
                    free(texture_coords);
                }
                if (normals) {
                    free(normals);
                }
                if (colors) {
                    free(colors);
                }
                material_map_free(material_map);
                return NULL;
            }
            
            current_group->indices = new_group_indices;
            memcpy(current_group->indices + old_size, indices, indices_size * sizeof(uint32_t));
            current_group->indices_size += indices_size;
            
            free(indices);
        }
        else if (strcmp(token, "mtllib") == 0) {
            // Material library reference
            char *mtl_filename = strtok(NULL, " \t\n\r");
            if (!mtl_filename) {
                continue;
            }
            
            char *mtl_path = combine_paths(base_path, mtl_filename);
            
            // Load materials from MTL file
            size_t materials_count = 0;
            struct mcc_obj_material *materials = mcc_obj_load_mtl(mtl_path, &materials_count);
            
            if (materials && materials_count > 0) {
                // Add materials to our loaded object
                struct mcc_obj_material *new_materials = realloc(loaded_obj->o_materials,
                                                             (loaded_obj->materials_size + materials_count) * 
                                                             sizeof(struct mcc_obj_material));
                if (!new_materials) {
                    free(mtl_path);
                    free(materials);
                    mcc_obj_loaded_obj_free(loaded_obj);
                    free(positions);
                    free(texture_coords);
                    free(normals);
                    return NULL;
                }
                
                loaded_obj->o_materials = new_materials;
                
                // Add materials to map
                for (size_t i = 0; i < materials_count; i++) {
                    loaded_obj->o_materials[loaded_obj->materials_size] = materials[i];
                    material_map_add(material_map, materials[i].r_name, loaded_obj->materials_size);
                    loaded_obj->materials_size++;
                }
            }
            
            free(mtl_path);
            free(materials); // We've copied the materials, so free the array
        }
        else if (strcmp(token, "usemtl") == 0) {
            // Material usage - set current group's material index
            // If we have indices, finalize current group and create a new one
            if (current_group && current_group->indices_size > 0) {
                struct mcc_obj_vertex_group *new_groups = realloc(current_object->o_vertex_groups,
                                                              (current_object->vertex_groups_size + 1) * 
                                                              sizeof(struct mcc_obj_vertex_group));
                if (!new_groups) {
                    mcc_obj_loaded_obj_free(loaded_obj);
                    free(positions);
                    free(texture_coords);
                    free(normals);
                    if (colors) {
                        free(colors);
                    }
                    material_map_free(material_map);
                    return NULL;
                }
                
                current_object->o_vertex_groups = new_groups;
                current_object->vertex_groups_size++;
                
                // Initialize new group
                mcc_obj_vertex_group_init(&current_object->o_vertex_groups[current_object->vertex_groups_size - 1]);
                current_group = &current_object->o_vertex_groups[current_object->vertex_groups_size - 1];
            }
            
            char *material_name = strtok(NULL, " \t\n\r");
            if (material_name) {
                // Use the material map to find the material index
                current_group->material_index = material_map_find(material_map, material_name);
            }
        }
    }
    
    // Handle case where there's data but we haven't created an object yet
    if (!current_object && (positions_size > 0 || texture_coords_size > 0 || normals_size > 0)) {
        struct mcc_obj_object *new_objects = realloc(loaded_obj->o_objects, 
                                                   (loaded_obj->objects_size + 1) * sizeof(struct mcc_obj_object));
        if (!new_objects) {
            mcc_obj_loaded_obj_free(loaded_obj);
            free(positions);
            if (texture_coords) {
                free(texture_coords);
            }
            if (normals) {
                free(normals);
            }
            if (colors) {
                free(colors);
            }
            material_map_free(material_map);
            return NULL;
        }
        
        loaded_obj->o_objects = new_objects;
        
        // Initialize the object directly in the array
        if (!mcc_obj_object_init(&loaded_obj->o_objects[loaded_obj->objects_size], "default")) {
            mcc_obj_loaded_obj_free(loaded_obj);
            free(positions);
            if (texture_coords) {
                free(texture_coords);
            }
            if (normals) {
                free(normals);
            }
            if (colors) {
                free(colors);
            }
            material_map_free(material_map);
            return NULL;
        }
        
        current_object = &loaded_obj->o_objects[loaded_obj->objects_size++];
    }
    
    // Finalize current object if it exists
    if (current_object) {
        if (positions_size > 0) {
            current_object->o_positions = malloc(positions_size * sizeof(mcc_vec4f));
            if (!current_object->o_positions) {
                mcc_obj_loaded_obj_free(loaded_obj);
                free(positions);
                if (texture_coords) {
                    free(texture_coords);
                }
                if (normals) {
                    free(normals);
                }
                if (colors) {
                    free(colors);
                }
                material_map_free(material_map);
                return NULL;
            }
            memcpy(current_object->o_positions, positions, positions_size * sizeof(mcc_vec4f));
            current_object->positions_size = positions_size;
        }
        
        if (texture_coords_size > 0) {
            current_object->o_texture_coordinates = malloc(texture_coords_size * sizeof(mcc_vec3f));
            if (!current_object->o_texture_coordinates) {
                mcc_obj_loaded_obj_free(loaded_obj);
                free(positions);
                if (texture_coords) {
                    free(texture_coords);
                }
                if (normals) {
                    free(normals);
                }
                if (colors) {
                    free(colors);
                }
                material_map_free(material_map);
                return NULL;
            }
            memcpy(current_object->o_texture_coordinates, texture_coords, texture_coords_size * sizeof(mcc_vec3f));
            current_object->texture_coordinates_size = texture_coords_size;
        }
        
        if (normals_size > 0) {
            current_object->o_normals = malloc(normals_size * sizeof(mcc_vec3f));
            if (!current_object->o_normals) {
                mcc_obj_loaded_obj_free(loaded_obj);
                free(positions);
                if (texture_coords) {
                    free(texture_coords);
                }
                if (normals) {
                    free(normals);
                }
                if (colors) {
                    free(colors);
                }
                material_map_free(material_map);
                return NULL;
            }
            memcpy(current_object->o_normals, normals, normals_size * sizeof(mcc_vec3f));
            current_object->normals_size = normals_size;
        }
        
        if (colors_size > 0) {
            current_object->o_colors = malloc(colors_size * sizeof(mcc_vec3f));
            if (!current_object->o_colors) {
                mcc_obj_loaded_obj_free(loaded_obj);
                free(positions);
                if (texture_coords) {
                    free(texture_coords);
                }
                if (normals) {
                    free(normals);
                }
                if (colors) {
                    free(colors);
                }
                material_map_free(material_map);
                return NULL;
            }
            memcpy(current_object->o_colors, colors, colors_size * sizeof(mcc_vec3f));
            current_object->colors_size = colors_size;
        }
    }
    
    // Free the material map and temporary buffers
    material_map_free(material_map);
    
    free(positions);
    free(texture_coords);
    free(normals);
    free(colors);
    
    return loaded_obj;
}

void mcc_obj_loaded_obj_free(struct mcc_obj_loaded_obj *obj) {
    if (!obj)
        return;
    
    for (size_t i = 0; i < obj->objects_size; i++) {
        struct mcc_obj_object *object = &obj->o_objects[i];
        
        free((char*)object->o_name);
        free(object->o_positions);
        free(object->o_texture_coordinates);
        free(object->o_normals);
        free(object->o_colors);
        
        for (size_t j = 0; j < object->vertex_groups_size; j++) {
            free(object->o_vertex_groups[j].indices);
        }
        
        free(object->o_vertex_groups);
    }
    
    free(obj->o_objects);
    
    for (size_t i = 0; i < obj->materials_size; i++) {
        mcc_obj_material_free(&obj->o_materials[i]);
    }
    
    free(obj->o_materials);
    
    free(obj);
}
