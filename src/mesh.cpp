#include "mesh.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstddef>
#include <cctype>

#include "texture.h"

namespace {
std::string toLower(std::string s)
{
    for (char& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

bool isWalkSurfaceName(const std::string& meshName)
{
    const std::string n = toLower(meshName);
    if (n.find("envelope") != std::string::npos) return false;
    if (n.find("vegetation") != std::string::npos) return false;
    return n.find("terrain") != std::string::npos ||
           n.find("road") != std::string::npos ||
           n.find("path") != std::string::npos ||
           n.find("pedestrian") != std::string::npos;
}

bool isBuildingName(const std::string& meshName)
{
    const std::string n = toLower(meshName);
    return n.find("building") != std::string::npos;
}

Mesh buildMesh(const aiScene* scene,
               const aiMesh* ai_mesh,
               const std::filesystem::path& modelDir,
               const aiMatrix4x4& worldTransform,
               Model& model,
               std::unordered_map<std::string, GLuint>& loadedTextures,
               glm::vec3& bboxMin,
               glm::vec3& bboxMax)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(ai_mesh->mNumVertices);
    indices.reserve(ai_mesh->mNumFaces * 3);

    aiMatrix3x3 normalMatrix(worldTransform);
    normalMatrix.Inverse().Transpose();

    for (unsigned int i = 0; i < ai_mesh->mNumVertices; ++i) {
        Vertex v{};
        const aiVector3D transformedPos = worldTransform * ai_mesh->mVertices[i];
        v.pos = glm::vec3(transformedPos.x, transformedPos.y, transformedPos.z);
        if (ai_mesh->HasNormals()) {
            aiVector3D transformedNormal = normalMatrix * ai_mesh->mNormals[i];
            const glm::vec3 n(transformedNormal.x, transformedNormal.y, transformedNormal.z);
            v.normal = (glm::length(n) > 1e-6f) ? glm::normalize(n) : glm::vec3(0.0f, 1.0f, 0.0f);
        } else {
            v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
        if (ai_mesh->HasTextureCoords(0)) {
            v.uv = glm::vec2(ai_mesh->mTextureCoords[0][i].x, ai_mesh->mTextureCoords[0][i].y);
        } else {
            v.uv = glm::vec2(0.0f);
        }

        bboxMin = glm::min(bboxMin, v.pos);
        bboxMax = glm::max(bboxMax, v.pos);
        vertices.push_back(v);
    }

    glm::vec3 meshMin(std::numeric_limits<float>::max());
    glm::vec3 meshMax(std::numeric_limits<float>::lowest());
    for (const auto& v : vertices) {
        meshMin = glm::min(meshMin, v.pos);
        meshMax = glm::max(meshMax, v.pos);
    }

    for (unsigned int f = 0; f < ai_mesh->mNumFaces; ++f) {
        const aiFace& face = ai_mesh->mFaces[f];
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
            indices.push_back(face.mIndices[j]);
        }
    }

    if (isWalkSurfaceName(ai_mesh->mName.C_Str())) {
        for (unsigned int f = 0; f < ai_mesh->mNumFaces; ++f) {
            const aiFace& face = ai_mesh->mFaces[f];
            if (face.mNumIndices != 3) continue;
            const Vertex& v0 = vertices[face.mIndices[0]];
            const Vertex& v1 = vertices[face.mIndices[1]];
            const Vertex& v2 = vertices[face.mIndices[2]];
            model.walkSurface.push_back({v0.pos, v1.pos, v2.pos});
        }
    }

    if (isBuildingName(ai_mesh->mName.C_Str())) {
        const float height = meshMax.y - meshMin.y;
        const float spanX = meshMax.x - meshMin.x;
        const float spanZ = meshMax.z - meshMin.z;
        if (height > 1.0f && spanX > 0.5f && spanZ > 0.5f) {
            model.obstacles.push_back({
                glm::vec2(meshMin.x, meshMin.z),
                glm::vec2(meshMax.x, meshMax.z),
                meshMin.y,
                meshMax.y
            });
        }
    }

    Mesh mesh{};
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glBindVertexArray(0);

    mesh.indexCount = static_cast<GLsizei>(indices.size());

    if (ai_mesh->mMaterialIndex < scene->mNumMaterials) {
        aiMaterial* mat = scene->mMaterials[ai_mesh->mMaterialIndex];

        aiColor3D color(1.0f, 1.0f, 1.0f);
        if (mat->Get(AI_MATKEY_BASE_COLOR, color) == AI_SUCCESS ||
            mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
            mesh.baseColor = glm::vec3(color.r, color.g, color.b);
        }

        aiString texPath;
        if (mat->GetTexture(aiTextureType_BASE_COLOR, 0, &texPath) != AI_SUCCESS) {
            mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath);
        }

        if (texPath.length > 0) {
            const std::string key = texPath.C_Str();
            const auto found = loadedTextures.find(key);
            if (found != loadedTextures.end()) {
                mesh.diffuseTex = found->second;
            } else {
                GLuint texId = 0;

                if (!key.empty() && key[0] == '*') {
                    const aiTexture* embedded = scene->GetEmbeddedTexture(key.c_str());
                    if (embedded) {
                        if (embedded->mHeight == 0) {
                            texId = loadTextureFromMemory(reinterpret_cast<const unsigned char*>(embedded->pcData), embedded->mWidth);
                        } else {
                            const std::size_t pixelCount = static_cast<std::size_t>(embedded->mWidth) * embedded->mHeight;
                            std::vector<unsigned char> rgba(pixelCount * 4);
                            for (std::size_t p = 0; p < pixelCount; ++p) {
                                rgba[p * 4 + 0] = embedded->pcData[p].r;
                                rgba[p * 4 + 1] = embedded->pcData[p].g;
                                rgba[p * 4 + 2] = embedded->pcData[p].b;
                                rgba[p * 4 + 3] = embedded->pcData[p].a;
                            }
                            texId = loadTextureFromMemory(rgba.data(), rgba.size());
                        }
                    }
                } else {
                    const std::filesystem::path fullPath = modelDir / key;
                    texId = loadTexture(fullPath.string());
                }

                if (texId != 0) {
                    loadedTextures[key] = texId;
                    mesh.diffuseTex = texId;
                }
            }
        }
    }

    return mesh;
}

void processNode(const aiScene* scene,
                 const aiNode* node,
                 const aiMatrix4x4& parentTransform,
                 const std::filesystem::path& modelDir,
                 std::unordered_map<std::string, GLuint>& loadedTextures,
                 Model& model,
                 glm::vec3& bboxMin,
                 glm::vec3& bboxMax)
{
    const aiMatrix4x4 worldTransform = parentTransform * node->mTransformation;

    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        model.meshes.push_back(buildMesh(scene, mesh, modelDir, worldTransform, model, loadedTextures, bboxMin, bboxMax));
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        processNode(scene, node->mChildren[i], worldTransform, modelDir, loadedTextures, model, bboxMin, bboxMax);
    }
}
}  // namespace

Model loadModel(const std::string& path)
{
    Model model{};

    Assimp::Importer importer;
    // PreTransformVertices flattens node transforms into final vertex positions.
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_FlipUVs);

    if (!scene || !scene->mRootNode || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)) {
        std::cerr << "Assimp load failed for " << path << ": " << importer.GetErrorString() << std::endl;
        return model;
    }

    const std::filesystem::path modelDir = std::filesystem::path(path).parent_path();
    std::unordered_map<std::string, GLuint> loadedTextures;

    glm::vec3 bboxMin(std::numeric_limits<float>::max());
    glm::vec3 bboxMax(std::numeric_limits<float>::lowest());
    processNode(scene, scene->mRootNode, aiMatrix4x4(), modelDir, loadedTextures, model, bboxMin, bboxMax);

    if (!model.meshes.empty()) {
        model.bboxMin = bboxMin;
        model.bboxMax = bboxMax;
        const glm::vec3 extent = model.bboxMax - model.bboxMin;
        model.sceneHasTerrain = (extent.x > 250.0f && extent.z > 250.0f);
    }

    std::size_t totalIndices = 0;
    std::size_t texturedMeshes = 0;
    for (const auto& mesh : model.meshes) {
        totalIndices += static_cast<std::size_t>(mesh.indexCount);
        if (mesh.diffuseTex != 0) texturedMeshes++;
    }
    std::cout << "Model stats: meshes=" << model.meshes.size()
              << ", triangles=" << (totalIndices / 3)
              << ", texturedMeshes=" << texturedMeshes
              << ", walkSurfaceTriangles=" << model.walkSurface.size()
              << ", obstacles=" << model.obstacles.size() << std::endl;

    return model;
}
