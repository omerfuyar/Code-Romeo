#include "tools/Resource.h"

#include "utilities/Timer.h"
#include "utilities/Maths.h"
#include "utilities/ListLinked.h"

#include "glad/glad.h"
#include "stb/stb_image.h"
#include "cgltf/cgltf.h"
#include "cglm/cglm.h"

#pragma region Source Only

// todo shifts on removal. find another way
ListLinked RESOURCE_TEXTURE_POOL = {0}; // ResourceTexture

#pragma region ResourceTexture

static RJ_ResultWarn ResourceTexture_GetByNameOrCreate(ResourceTexture **retTexture, StringView name, StringView filePathInResources)
{
    for (RJ_Size textureIndex = 0; textureIndex < RESOURCE_TEXTURE_POOL.count; textureIndex++)
    {
        ResourceTexture *texture = (ResourceTexture *)ListLinked_Get(&RESOURCE_TEXTURE_POOL, textureIndex);

        if (String_AreSame(scv(texture->name), name))
        {
            texture->refCount++;
            *retTexture = texture;
            return RJ_OK;
        }
    }

    ResourceTexture *texture = (ResourceTexture *)ListLinked_Add(&RESOURCE_TEXTURE_POOL, NULL);
    *retTexture = texture;

    texture->index = RESOURCE_TEXTURE_POOL.count - 1;
    texture->name = scc(name);
    texture->refCount = 1;

    String tempFilePath = scc(filePathInResources);
    String_ConcatEnd(&tempFilePath, scv(texture->name));

    ResourceImage *image = NULL;
    RJ_Result result = ResourceImage_Create(&image, scv(tempFilePath));
    String_Destroy(&tempFilePath);
    if (result != RJ_OK)
    {
        ListLinked_RemoveAtIndex(&RESOURCE_TEXTURE_POOL, texture->index);
        String_Destroy(&texture->name);
        RJ_DebugWarning("Failed to create resource image for texture '%s'.", texture->name.characters);
        return result;
    }

    glGenTextures(1, &texture->handle);
    glBindTexture(GL_TEXTURE_2D, texture->handle);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLenum format = 0;

    switch (image->channels)
    {
    case 4:
        format = GL_RGBA;
        break;
    case 3:
        format = GL_RGB;
        break;
    case 2:
        format = GL_RG;
        break;
    case 1:
        format = GL_RED;
        break;
    default:
        RJ_DebugWarning("Unsupported number of channels (%d) for texture '%s'.", image->channels, texture->name.characters);
        ResourceImage_Destroy(image);
        return RJ_ERROR_RESOURCE;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, (GLint)format, image->size.x, image->size.y, 0, format, GL_UNSIGNED_BYTE, image->data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    ResourceImage_Destroy(image);

    RJ_DebugInfo("Texture '%s' created successfully.", texture->name.characters);

    return RJ_OK;
}

static RJ_ResultWarn ResourceTexture_GetFromMemoryOrCreate(ResourceTexture **retTexture, StringView name, const void *bufferData, size_t bufferSize)
{
    for (RJ_Size textureIndex = 0; textureIndex < RESOURCE_TEXTURE_POOL.count; textureIndex++)
    {
        ResourceTexture *texture = (ResourceTexture *)ListLinked_Get(&RESOURCE_TEXTURE_POOL, textureIndex);

        if (String_AreSame(scv(texture->name), name))
        {
            texture->refCount++;
            *retTexture = texture;
            return RJ_OK;
        }
    }

    ResourceTexture *texture = (ResourceTexture *)ListLinked_Add(&RESOURCE_TEXTURE_POOL, NULL);
    *retTexture = texture;

    texture->index = RESOURCE_TEXTURE_POOL.count - 1;
    texture->name = scc(name);
    texture->refCount = 1;

    int x, y, channels;
    unsigned char *data = stbi_load_from_memory((const unsigned char *)bufferData, (int)bufferSize, &x, &y, &channels, 0);
    if (!data)
    {
        ListLinked_RemoveAtIndex(&RESOURCE_TEXTURE_POOL, texture->index);
        String_Destroy(&texture->name);
        RJ_DebugWarning("Failed to load embedded resource image for texture '%s'.", texture->name.characters);
        return RJ_ERROR_RESOURCE;
    }

    glGenTextures(1, &texture->handle);
    glBindTexture(GL_TEXTURE_2D, texture->handle);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLenum format = 0;

    switch (channels)
    {
    case 4:
        format = GL_RGBA;
        break;
    case 3:
        format = GL_RGB;
        break;
    case 2:
        format = GL_RG;
        break;
    case 1:
        format = GL_RED;
        break;
    default:
        RJ_DebugWarning("Unsupported number of channels (%d) for texture '%s'.", channels, texture->name.characters);
        stbi_image_free(data);
        return RJ_ERROR_RESOURCE;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, (GLint)format, x, y, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);

    RJ_DebugInfo("Texture '%s' created successfully.", texture->name.characters);

    return RJ_OK;
}

static void ResourceTexture_Destroy(ResourceTexture *texture)
{
    RJ_DebugAssertNullPointerCheck(texture);

    texture->refCount--;
    if (texture->refCount > 0)
    {
        return;
    }

    char tempTitle[RJ_TEMP_BUFFER_SIZE] = {0};
    scb(texture->name, tempTitle, sizeof(tempTitle));

    String_Destroy(&texture->name);
    glDeleteTextures(1, &texture->handle);
    ListLinked_RemoveAtIndex(&RESOURCE_TEXTURE_POOL, texture->index);

    RJ_DebugInfo("Resource Texture '%s' destroyed successfully.", tempTitle);
}

#pragma endregion ResourceTexture

#pragma endregion Source Only

void Resource_Initialize(void)
{
    stbi_set_flip_vertically_on_load(true);
    RESOURCE_TEXTURE_POOL = ListLinked_Create("Resource Texture", sizeof(ResourceTexture));
}

void Resource_Terminate(void)
{
    // todo cleanup
    ListLinked_Destroy(&RESOURCE_TEXTURE_POOL);
}

bool Resource_IsInitialized(void)
{
    return RESOURCE_TEXTURE_POOL.head != NULL;
}

#pragma region ResourceText

RJ_ResultWarn ResourceText_Create(ResourceText **retResource, StringView file)
{
    ResourceText *resource = *retResource;

    RJ_ReturnAllocate(ResourceText, resource, 1);

    *retResource = resource;

    resource->file = scc(file);

    // RJ_Size pathCount = 0;
    // String pathBuffer[RJ_TEMP_BUFFER_SIZE / 32];
    // String_Tokenize(relativePath, scl(RJ_PATH_DELIMETER_STR), &pathCount, pathBuffer, RJ_TEMP_BUFFER_SIZE / 32);
    //
    // for (RJ_Size i = 0; i < pathCount - 1; i++)
    //{
    //    String_ConcatEnd(&resource->path, pathBuffer[i]);
    //}

    String fullPath = scc(resource->file);
    String_ConcatBegin(&fullPath, scl(RESOURCE_PATH));
    String_ConcatBegin(&fullPath, scl(RJ_GetExecutablePath()));
    swn(&fullPath);

    RJ_Size lineCount = 0;
    int character = 0;

    FILE *fileHandle = NULL;
    RJ_ReturnFileOpen(fileHandle, fullPath.characters, "r",
                      String_Destroy(&fullPath);
                      free(resource););

    while ((character = fgetc(fileHandle)) != EOF)
    {
        if (character == '\n')
        {
            lineCount++;
        }
    }

    resource->lineCount = lineCount + 1;

    rewind(fileHandle);

    // on heap because the buffer is too large for stack
    char *dataBuffer = NULL;
    RJ_ReturnAllocate(char, dataBuffer, resource->lineCount *RESOURCE_FILE_LINE_MAX_CHAR_COUNT,
                      String_Destroy(&fullPath);
                      free(resource););

    dataBuffer[0] = '\0';

    char lineBuffer[RESOURCE_FILE_LINE_MAX_CHAR_COUNT] = {0};

    RJ_Size dataIndex = 0;

    while (fgets(lineBuffer, RESOURCE_FILE_LINE_MAX_CHAR_COUNT, fileHandle))
    {
        RJ_Size lineLength = (RJ_Size)strlen(lineBuffer);
        memcpy(dataBuffer + dataIndex, lineBuffer, lineLength);
        dataIndex += lineLength;
    }
    fclose(fileHandle);

    dataBuffer[dataIndex] = '\0';

    resource->data = scc(scs(dataBuffer, dataIndex));

    free(dataBuffer);

    String_Destroy(&fullPath);

    RJ_DebugInfo("Resource text '%s' loaded successfully.", resource->file.characters);

    return RJ_OK;
}

void ResourceText_Destroy(ResourceText *resource)
{
    RJ_DebugAssertNullPointerCheck(resource);
    RJ_DebugAssertNullPointerCheck(resource->file.characters);

    char tempTitle[RJ_TEMP_BUFFER_SIZE] = {0};
    scb(resource->file, tempTitle, sizeof(tempTitle));

    String_Destroy(&resource->file);
    String_Destroy(&resource->data);
    resource->lineCount = 0;

    if (resource != NULL)
    {
        free(resource);
        resource = NULL;
    }

    RJ_DebugInfo("Resource Text '%s' destroyed successfully.", tempTitle);
}

#pragma endregion ResourceText

#pragma region ResourceImage

RJ_ResultWarn ResourceImage_Create(ResourceImage **retResourceImage, StringView file)
{
    ResourceImage *resourceImage = *retResourceImage;

    RJ_ReturnAllocate(ResourceImage, resourceImage, 1);

    *retResourceImage = resourceImage;

    resourceImage->file = scc(file);

    String fullPath = scc(resourceImage->file);
    String_ConcatBegin(&fullPath, scl(RESOURCE_PATH));
    String_ConcatBegin(&fullPath, scl(RJ_GetExecutablePath()));
    swn(&fullPath);

    resourceImage->data = stbi_load(fullPath.characters, &resourceImage->size.x, &resourceImage->size.y, &resourceImage->channels, 0);
    if (resourceImage->data == NULL)
    {
        String_Destroy(&fullPath);
        free(resourceImage);
        RJ_DebugWarning("Failed to load image data from file '%s'. STB Error: %s", fullPath.characters, stbi_failure_reason());
        return RJ_ERROR_DEPENDENCY;
    }

    String_Destroy(&fullPath);

    RJ_DebugInfo("Resource Image '%s' loaded successfully.", resourceImage->file.characters);

    return RJ_OK;
}

void ResourceImage_Destroy(ResourceImage *resourceImage)
{
    RJ_DebugAssertNullPointerCheck(resourceImage);

    char tempTitle[RJ_TEMP_BUFFER_SIZE] = {0};
    scb(resourceImage->file, tempTitle, sizeof(tempTitle));

    String_Destroy(&resourceImage->file);
    resourceImage->channels = 0;
    resourceImage->size = Vector2Int_New(0, 0);

    stbi_image_free(resourceImage->data);
    resourceImage->data = NULL;

    free(resourceImage);
    resourceImage = NULL;

    RJ_DebugInfo("Resource Image '%s' destroyed successfully.", tempTitle);
}

#pragma endregion ResourceImage

#pragma region ResourceModel

static void ProcessNode(cgltf_node *node, mat4 parentTransform, ResourceModel *model, cgltf_data *data, ResourceMesh **meshMap)
{
    mat4 localTransform;
    cgltf_node_transform_local(node, (cgltf_float *)localTransform);

    mat4 globalTransform;
    glm_mat4_mul(parentTransform, localTransform, globalTransform);

    if (node->mesh)
    {
        for (cgltf_size primitiveIndex = 0; primitiveIndex < node->mesh->primitives_count; primitiveIndex++)
        {
            cgltf_primitive *primitive = &node->mesh->primitives[primitiveIndex];
            ResourceMesh *targetMesh = meshMap[0]; // fallback
            if (primitive->material)
            {
                // find index
                for (cgltf_size m = 0; m < data->materials_count; m++)
                {
                    if (&data->materials[m] == primitive->material)
                    {
                        targetMesh = (ResourceMesh *)ListArray_Get(&model->meshes, (RJ_Size)m);
                        break;
                    }
                }
            }

            cgltf_accessor *posAcc = NULL;
            cgltf_accessor *normAcc = NULL;
            cgltf_accessor *uvAcc = NULL;

            for (cgltf_size a = 0; a < primitive->attributes_count; a++)
            {
                if (primitive->attributes[a].type == cgltf_attribute_type_position)
                    posAcc = primitive->attributes[a].data;
                else if (primitive->attributes[a].type == cgltf_attribute_type_normal)
                    normAcc = primitive->attributes[a].data;
                else if (primitive->attributes[a].type == cgltf_attribute_type_texcoord && primitive->attributes[a].index == 0)
                    uvAcc = primitive->attributes[a].data;
            }

            if (!posAcc)
                continue;

            RJ_Size vertexOffset = model->vertices.count;

            for (cgltf_size v = 0; v < posAcc->count; v++)
            {
                ResourceMeshVertex vertex = {0};
                float pos[3] = {0};
                cgltf_accessor_read_float(posAcc, v, pos, 3);
                vec4 posVec = {pos[0], pos[1], pos[2], 1.0f};
                glm_mat4_mulv(globalTransform, posVec, posVec);
                vertex.position = Vector3_New(posVec[0], posVec[1], posVec[2]);

                if (normAcc)
                {
                    float norm[3] = {0};
                    cgltf_accessor_read_float(normAcc, v, norm, 3);
                    vec3 normVec = {norm[0], norm[1], norm[2]};
                    mat3 normalMatrix;

                    mat4 inv;
                    glm_mat4_inv(globalTransform, inv);
                    glm_mat4_transpose(inv);
                    glm_mat4_pick3(inv, normalMatrix);

                    glm_mat3_mulv(normalMatrix, normVec, normVec);
                    glm_vec3_normalize(normVec);

                    vertex.normal = Vector3_New(normVec[0], normVec[1], normVec[2]);
                }

                if (uvAcc)
                {
                    float uv[2] = {0};
                    cgltf_accessor_read_float(uvAcc, v, uv, 2);
                    vertex.uv = Vector2_New(uv[0], 1.0f - uv[1]);
                }

                ListArray_Add(&model->vertices, &vertex);
            }

            if (primitive->indices)
            {
                for (cgltf_size i = 0; i < primitive->indices->count; i++)
                {
                    ResourceMeshIndex idx = (ResourceMeshIndex)(cgltf_accessor_read_index(primitive->indices, i) + vertexOffset);
                    ListArray_Add(&targetMesh->indices, &idx);
                }
            }
            else
            {
                for (cgltf_size i = 0; i < posAcc->count; i++)
                {
                    ResourceMeshIndex idx = (ResourceMeshIndex)(i + vertexOffset);
                    ListArray_Add(&targetMesh->indices, &idx);
                }
            }
        }
    }

    for (cgltf_size c = 0; c < node->children_count; c++)
    {
        ProcessNode(node->children[c], globalTransform, model, data, meshMap);
    }
}

RJ_ResultWarn ResourceModel_Create(ResourceModel **retResourceModel, StringView fileName)
{
    ResourceModel *model = *retResourceModel;
    RJ_ReturnAllocate(ResourceModel, model, 1);
    *retResourceModel = model;

    model->file = scc(fileName);

    String fullPath = scc(model->file);
    String_ConcatBegin(&fullPath, scl(RESOURCE_PATH));
    String_ConcatBegin(&fullPath, scl(RJ_GetExecutablePath()));
    swn(&fullPath);

    cgltf_options options = {0};
    cgltf_data *data = NULL;
    cgltf_result result = cgltf_parse_file(&options, fullPath.characters, &data);

    if (result != cgltf_result_success)
    {
        String_Destroy(&fullPath);
        free(model);
        *retResourceModel = NULL;
        RJ_DebugWarning("Failed to parse GLTF file '%s'. cgltf Error: %d", fullPath.characters, result);
        return RJ_ERROR_DEPENDENCY;
    }

    result = cgltf_load_buffers(&options, data, fullPath.characters);
    if (result != cgltf_result_success)
    {
        cgltf_free(data);
        String_Destroy(&fullPath);
        free(model);
        *retResourceModel = NULL;
        RJ_DebugWarning("Failed to load GLTF buffers '%s'. cgltf Error: %d", fullPath.characters, result);
        return RJ_ERROR_DEPENDENCY;
    }

    String_Destroy(&fullPath);

    ListArray_Create(&model->vertices, "Model Vertices", sizeof(ResourceMeshVertex), 1024);
    ListArray_Create(&model->meshes, "Model Meshes", sizeof(ResourceMesh), data->materials_count > 0 ? data->materials_count : 1);

    // Map each material into a ResourceMesh
    ResourceMesh **meshMap = NULL;
    RJ_Size materialCount = data->materials_count > 0 ? (RJ_Size)data->materials_count : 1;
    meshMap = (ResourceMesh **)calloc(materialCount, sizeof(ResourceMesh *));

    for (cgltf_size i = 0; i < data->materials_count; i++)
    {
        ResourceMesh mesh = {0};
        ListArray_Create(&mesh.indices, "Mesh Indices", sizeof(ResourceMeshIndex), 1024);

        ResourceMaterial *mat = (ResourceMaterial *)calloc(1, sizeof(ResourceMaterial));
        mat->name = scc(scl(data->materials[i].name ? data->materials[i].name : "default"));

        if (data->materials[i].has_pbr_metallic_roughness)
        {
            mat->baseColorFactor = Vector4_New(
                data->materials[i].pbr_metallic_roughness.base_color_factor[0],
                data->materials[i].pbr_metallic_roughness.base_color_factor[1],
                data->materials[i].pbr_metallic_roughness.base_color_factor[2],
                data->materials[i].pbr_metallic_roughness.base_color_factor[3]);
            mat->metallicFactor = data->materials[i].pbr_metallic_roughness.metallic_factor;
            mat->roughnessFactor = data->materials[i].pbr_metallic_roughness.roughness_factor;

            cgltf_texture_view *baseTextureView = &data->materials[i].pbr_metallic_roughness.base_color_texture;
            if (baseTextureView->texture && baseTextureView->texture->image)
            {
                String dirPath = scc(model->file);
                for (RJ_Size idx = dirPath.length; idx > 0; idx--)
                {
                    if (dirPath.characters[idx - 1] == '/' || dirPath.characters[idx - 1] == '\\')
                    {
                        dirPath.length = idx;
                        break;
                    }
                }
                if (dirPath.length == model->file.length)
                    dirPath.length = 0;

                if (baseTextureView->texture->image->uri)
                {
                    ResourceTexture_GetByNameOrCreate(&mat->baseColorMap, scl(baseTextureView->texture->image->uri), scv(dirPath));
                }
                else if (baseTextureView->texture->image->buffer_view && baseTextureView->texture->image->buffer_view->buffer->data)
                {
                    char embName[64];
                    snprintf(embName, sizeof(embName), "emb_%zu_%zu", (size_t)baseTextureView->texture->image->buffer_view->offset, (size_t)baseTextureView->texture->image->buffer_view->size);
                    const void *dataPtr = (const unsigned char *)baseTextureView->texture->image->buffer_view->buffer->data + baseTextureView->texture->image->buffer_view->offset;
                    ResourceTexture_GetFromMemoryOrCreate(&mat->baseColorMap, scl(embName), dataPtr, baseTextureView->texture->image->buffer_view->size);
                }
                String_Destroy(&dirPath);
            }

            cgltf_texture_view *mrTextureView = &data->materials[i].pbr_metallic_roughness.metallic_roughness_texture;
            if (mrTextureView->texture && mrTextureView->texture->image)
            {
                String dirPath = scc(model->file);
                for (RJ_Size idx = dirPath.length; idx > 0; idx--)
                {
                    if (dirPath.characters[idx - 1] == '/' || dirPath.characters[idx - 1] == '\\')
                    {
                        dirPath.length = idx;
                        break;
                    }
                }
                if (dirPath.length == model->file.length)
                    dirPath.length = 0;

                if (mrTextureView->texture->image->uri)
                {
                    ResourceTexture_GetByNameOrCreate(&mat->metallicRoughnessMap, scl(mrTextureView->texture->image->uri), scv(dirPath));
                }
                else if (mrTextureView->texture->image->buffer_view && mrTextureView->texture->image->buffer_view->buffer->data)
                {
                    char embName[64];
                    snprintf(embName, sizeof(embName), "emb_%zu_%zu", (size_t)mrTextureView->texture->image->buffer_view->offset, (size_t)mrTextureView->texture->image->buffer_view->size);
                    const void *dataPtr = (const unsigned char *)mrTextureView->texture->image->buffer_view->buffer->data + mrTextureView->texture->image->buffer_view->offset;
                    ResourceTexture_GetFromMemoryOrCreate(&mat->metallicRoughnessMap, scl(embName), dataPtr, mrTextureView->texture->image->buffer_view->size);
                }
                String_Destroy(&dirPath);
            }
        }
        else
        {
            mat->baseColorFactor = Vector4_New(1, 1, 1, 1);
            mat->metallicFactor = 1.0f;
            mat->roughnessFactor = 1.0f;
        }

        mesh.material = mat;
        meshMap[i] = (ResourceMesh *)ListArray_Add(&model->meshes, &mesh);
    }

    if (data->materials_count == 0)
    {
        ResourceMesh mesh = {0};
        ListArray_Create(&mesh.indices, "Mesh Indices", sizeof(ResourceMeshIndex), 1024);
        ResourceMaterial *mat = (ResourceMaterial *)calloc(1, sizeof(ResourceMaterial));
        mat->name = scc(scl("default"));
        mat->baseColorFactor = Vector4_New(1, 1, 1, 1);
        mat->metallicFactor = 1.0f;
        mat->roughnessFactor = 1.0f;
        mesh.material = mat;
        meshMap[0] = (ResourceMesh *)ListArray_Add(&model->meshes, &mesh);
    }

    mat4 offsetMatrix;
    glm_mat4_identity(offsetMatrix);

    if (data->scene)
    {
        for (cgltf_size n = 0; n < data->scene->nodes_count; n++)
        {
            ProcessNode(data->scene->nodes[n], offsetMatrix, model, data, meshMap);
        }
    }
    else if (data->nodes_count > 0)
    {
        ProcessNode(&data->nodes[0], offsetMatrix, model, data, meshMap);
    }

    free(meshMap);
    cgltf_free(data);

    RJ_DebugInfo("Resource Model '%s' loaded successfully.", model->file.characters);

    return RJ_OK;
}

void ResourceModel_Destroy(ResourceModel *model)
{
    RJ_DebugAssertNullPointerCheck(model);
    char tempTitle[RJ_TEMP_BUFFER_SIZE] = {0};
    scb(model->file, tempTitle, sizeof(tempTitle));

    for (RJ_Size i = 0; i < model->meshes.count; i++)
    {
        ResourceMesh *mesh = (ResourceMesh *)ListArray_Get(&model->meshes, i);
        if (mesh && mesh->material)
        {
            if (mesh->material->baseColorMap)
            {
                ResourceTexture_Destroy(mesh->material->baseColorMap);
            }
            if (mesh->material->metallicRoughnessMap)
            {
                ResourceTexture_Destroy(mesh->material->metallicRoughnessMap);
            }
            free(mesh->material);
        }
        if (mesh)
        {
            ListArray_Destroy(&mesh->indices);
        }
    }

    ListArray_Destroy(&model->meshes);
    ListArray_Destroy(&model->vertices);
    String_Destroy(&model->file);

    free(model);

    RJ_DebugInfo("Resource Model '%s' destroyed successfully.", tempTitle);
}

#pragma endregion ResourceModel
