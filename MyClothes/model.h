#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "std_image.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Mesh.h"
#include "shader.h"
#include "Spring.h"
#include <cstdio>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <map>
#include <set>
#include <vector>
#include <array>

using namespace std;

unsigned int TextureFromFile(const char* path, const string& directory, bool gamma = false);

struct Triple
{
    int firstVertex;
    int secondVertex;
    int faceIndex;
};

struct Edge
{
    int firstVertex;
    int secondVertex;
};

struct Neighbour 
{
    int firstTriangle;
    int secondTriangle;
};
bool cmp(Triple t1, Triple t2)
{
    if (t1.firstVertex != t2.firstVertex)
    {
        return t1.firstVertex < t2.firstVertex;
    }
    else
    {
        return t1.secondVertex < t2.secondVertex;
    }
}

class Model
{
public:
    // model data 
    vector<Texture> textures_loaded;	// stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
    vector<Mesh>    meshes;
    vector<aiFace> temp_faces;
    vector<int> index_Normal;
    vector<int> index_Tex;
    vector<glm::vec3> sortedPosition;
    vector<glm::vec3> position;
    vector<Triple> triple;
    vector<Edge> edge;
    vector<Edge> diagonal;
    vector<Neighbour> nei;
    string directory;
    bool gammaCorrection;

    //Springs
    glm::vec3 gravity = glm::vec3(0, -0.98, 0);
    float k = 2;
    float f = 0.3;
    float structuralCoef = 1000.0;
    float bendingCoef = 400.0;
    float dampCoef = 1.0;

    bool isFalling = true;
    bool isRotating = false;
    bool stopCollsion = false;
    vector<Spring> springs;
    float ballVelocity = 5;
    float ballacc = 0.2;
    float staticU = 0.6f;
    float dynamicU = 0.3f;
    // constructor, expects a filepath to a 3D model.
    Model(string const& path, bool gamma = false) : gammaCorrection(gamma)
    {
        loadModel(path);
    }

    // draws the model, and thus all its meshes
    void Draw(Shader& shader)
    {
        for (unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);

    }
    void read() 
    {
        vector<Vertex> sortedVertex;
        for (int i = 0; i < meshes.size(); i++)
        {
            for (int j = 0; j < meshes[i].vertices.size(); j++)
            {
                bool add = true;
                for (int k = 0; k < j; k++)
                {
                    if (meshes[i].vertices[j].Position == meshes[i].vertices[k].Position)
                        add = false;
                }
                if (add == true)
                {
                    sortedVertex.push_back(meshes[i].vertices[j]);
                }
            }
        }

        for (int i = 0; i < meshes.size(); i++) 
        {
            for (int j = 0; j < meshes[i].indices.size(); j++) 
            {
                glm::vec3 temp_position = meshes[i].vertices[meshes[i].indices[j]].Position;
                for (int k = 0; k < sortedVertex.size(); k++) 
                {
                    if (temp_position == sortedVertex[k].Position) 
                    {
                        meshes[i].indices[j] = k;
                    }
                }
            }
        }
        for (int i = 0; i < meshes.size(); i++)
        {
            meshes[i] = Mesh(sortedVertex, meshes[i].indices, meshes[i].textures);
        }
    }
    void Write(string file_name)
    {
        ofstream dataFile;
        dataFile.open(file_name, ofstream::app);
        fstream file(file_name, ios::out);
        int i, j;
        vector<glm::vec3> temp_Position;
        vector<glm::vec3> temp_Normal;
        vector<glm::vec2> temp_tex;
        vector<unsigned int> temp_indices;
        for (i = 0; i < meshes.size(); i++) 
        {
            for (j = 0; j < meshes[i].vertices.size(); j++) 
            {
                temp_Position.push_back(meshes[i].vertices[j].Position);
                temp_Normal.push_back(meshes[i].vertices[j].Normal);
                temp_tex.push_back(meshes[i].vertices[j].TexCoords);
                temp_indices.push_back(meshes[i].indices[j]);
            }
        }

        position = temp_Position;
        for (i = 0; i < temp_Position.size(); i++) 
        {
            dataFile << "V" << " " << float(temp_Position[i].x) <<" "<< float(temp_Position[i].y) << " " << float(temp_Position[i].z) << endl;
        }

        for (i = 0; i < temp_tex.size(); i++)
        {
            dataFile << "Vt" << " " << float(temp_tex[i].x) << " " << float(temp_tex[i].y) << endl;
        }

        for (i = 0; i < temp_Normal.size(); i++)
        {
            dataFile << "Vn" << " " << float(temp_Normal[i].x) << " " << float(temp_Normal[i].y) << " " << float(temp_Normal[i].z) << endl;
        }

        for (i = 0; i < meshes.size(); i++) 
        {
            for (j = 0; j < meshes[i].indices.size()/3; j++)
            {
                int k,temp1,temp2;
                for (k = 0; k < temp_Normal.size(); k++)
                {
                    if (temp_Normal[k] == temp_Normal[index_Normal[k]]) temp1 = index_Normal[k];
                }
                for (k = 0; k < temp_tex.size(); k++)
                {
                    if (temp_tex[k] == temp_tex[index_Normal[k]]) temp2 = index_Tex[k];
                }
                dataFile << "f" << " " << meshes[i].indices[j] << "/" << temp2 << "/" << temp1 <<" ";
            }
            dataFile << " " << endl;
        }
        dataFile.close();
    }
    void vertexSort() 
    {
        for (int k = 0; k < meshes.size(); k++) 
        {
            for (int i = 0; i < meshes[k].indices.size()/3; i++)
            {
                Triple tempTriple;
                tempTriple.firstVertex = meshes[k].indices[3 * i];
                tempTriple.secondVertex = meshes[k].indices[3 * i + 1];
                tempTriple.faceIndex = i;
                triple.push_back(tempTriple);
                tempTriple.firstVertex = meshes[k].indices[3 * i + 1];
                tempTriple.secondVertex = meshes[k].indices[3 * i + 2];
                tempTriple.faceIndex = i;
                triple.push_back(tempTriple);
                tempTriple.firstVertex = meshes[k].indices[3 * i + 2];
                tempTriple.secondVertex = meshes[k].indices[3 * i];
                tempTriple.faceIndex = i;
                triple.push_back(tempTriple);
            }
        }

        //sort
        for (int i = 0; i < triple.size(); i++) 
        {
            if (triple[i].firstVertex > triple[i].secondVertex) 
            {
                int temp = triple[i].secondVertex;
                triple[i].secondVertex = triple[i].firstVertex;
                triple[i].firstVertex = temp;
            }
        }
        sort(triple.begin(), triple.end(), cmp);

        //removal
        for (int i = 0; i < triple.size()-1; i++) 
        {
            for (int j = i+1; j < triple.size(); j++) 
            {
                if ((triple[j].firstVertex == triple[i].firstVertex) && (triple[j].secondVertex == triple[i].secondVertex)
                    || (triple[j].firstVertex == triple[i].secondVertex) && (triple[j].secondVertex == triple[i].firstVertex))
                {
                    Neighbour n;
                    n.firstTriangle = triple[i].faceIndex;
                    n.secondTriangle = triple[j].faceIndex;
                    nei.push_back(n);

                    Triple temp = triple[triple.size()-1];
                    triple[triple.size() - 1] = triple[j];
                    triple[j] = temp;
                    triple.pop_back();

                }
            }
        }
        for (int i = 0; i < triple.size() - 1; i++)
        {
            for (int j = i + 1; j < triple.size(); j++)
            {
                if ((triple[j].firstVertex == triple[i].firstVertex) && (triple[j].secondVertex == triple[i].secondVertex)
                    || (triple[j].firstVertex == triple[i].secondVertex) && (triple[j].secondVertex == triple[i].firstVertex))
                {
                    Neighbour n;
                    n.firstTriangle = triple[i].faceIndex;
                    n.secondTriangle = triple[j].faceIndex;
                    nei.push_back(n);

                    Triple temp = triple[triple.size() - 1];
                    triple[triple.size() - 1] = triple[j];
                    triple[j] = temp;
                    triple.pop_back();

                }
            }
        }

        //edge list
        for (int i = 0; i < triple.size(); i++) 
        {
            Edge e;
            e.firstVertex = triple[i].firstVertex;
            e.secondVertex = triple[i].secondVertex;
            edge.push_back(e);
        }

        //diagonal list
        for (int i = 0; i<nei.size(); i++)
        {
            for (int j = 0; j < meshes.size(); j++) 
            {
                int index_01 = 3 * nei[i].firstTriangle;
                int index_02 = 3 * nei[i].secondTriangle;
                Edge temp_edge;
                for (int k = 0; k < 3; k++) 
                {
                    if (meshes[j].indices[index_01 + k] != meshes[j].indices[index_02])
                    {
                        if (meshes[j].indices[index_01 + k] != meshes[j].indices[index_02 + 1])
                        {
                            if (meshes[j].indices[index_01 + k] != meshes[j].indices[index_02 + 2])
                            {
                                temp_edge.firstVertex = meshes[j].indices[index_01 + k];
                            }
                        }
                    }

                    if (meshes[j].indices[index_02 + k] != meshes[j].indices[index_01])
                    {
                        if (meshes[j].indices[index_02 + k] != meshes[j].indices[index_01 + 1])
                        {
                            if (meshes[j].indices[index_02 + k] != meshes[j].indices[index_01 + 2])
                            {
                                temp_edge.secondVertex = meshes[j].indices[index_02 + k];
                            }
                        }
                    }
                }


                diagonal.push_back(temp_edge);
            }
            
        }
        
        //cout << "diagonal list" << endl;
        for (int i = 0; i < diagonal.size(); i++) 
        {
            //cout << diagonal[i].firstVertex << " " << diagonal[i].secondVertex << " " << endl;
        }

        //neighbour list
        //cout << "Neighbour list" << endl;
        for (int i = 0; i < nei.size(); i++) 
        {
           // cout << nei[i].firstTriangle << " " << nei[i].secondTriangle << " " << endl;
        }

        //output
        //cout << "Edge list" << endl;
        for (int i = 0; i < edge.size(); i++) 
        {
            //cout << edge[i].firstVertex << " " << edge[i].secondVertex << " "<< endl;
        }
    }

    void CreateSpring() 
    {
        for (int i = 0; i < meshes.size(); i++)
        {
            //Structural
            for (int j = 0; j < edge.size(); j++) 
            {
                springs.push_back(Spring(&(meshes[i].vertices[edge[j].firstVertex]), &(meshes[i].vertices[edge[j].secondVertex]), structuralCoef, dampCoef));
                //SimulateSpring(meshes[i].vertices[edge[j].firstVertex], meshes[i].vertices[edge[j].secondVertex],0.016f, structuralCoef);
            }

            //Blending
            for (int j = 0; j < diagonal.size(); j++)
            {
                springs.push_back(Spring(&(meshes[i].vertices[diagonal[j].firstVertex]), &(meshes[i].vertices[diagonal[j].secondVertex]), bendingCoef, dampCoef));
                //SimulateSpring(meshes[i].vertices[diagonal[j].firstVertex], meshes[i].vertices[diagonal[j].secondVertex], 0.016f, bendingCoef);
            }
        }

    }

    void SimulateNodes(float etha) 
    { 
        for (int i = 0; i < meshes.size(); i++) 
        {
            for (int j = 0; j < meshes[i].vertices.size(); j++)
            {
                if (meshes[i].vertices[j].isFixed) continue;

                meshes[i].vertices[j].acceleration = meshes[i].vertices[j].force / meshes[i].vertices[j].mass;
                meshes[i].vertices[j].velocity += meshes[i].vertices[j].acceleration * etha;

                glm::vec3 temp = meshes[i].vertices[j].Position;
                meshes[i].vertices[j].Position += (meshes[i].vertices[j].Position - meshes[i].vertices[j].oldPos) + meshes[i].vertices[j].velocity * etha;
                meshes[i].vertices[j].oldPos = temp;

                meshes[i].vertices[j].force = glm::vec3(0);   // reset
            }
        }
    }

    void SimulateInternalForce(float etha) 
    { 
        for (int i = 0;i< springs.size();i++)
        {
            springs[i].Simulate(etha);
        }
    }

    void SimulateGravity(void) 
    {
        for (int i = 0; i < meshes.size(); i++)
        {
            for (int j = 0; j < meshes[i].vertices.size(); j++)
            {
                meshes[i].vertices[j].force += (gravity * meshes[i].vertices[j].mass) * 20.0f;
            }
        }
    }

    void updatePosition()
    {
        for (int i = 0; i < meshes.size(); i++)
        {
            meshes[i] = Mesh(meshes[i].vertices, meshes[i].indices, meshes[i].textures);
        }

        for (int i = 0; i < meshes.size(); i++)
        {
            for (int k = 0; k < springs.size(); k++) 
            {
                for (int j = 0; j < edge.size(); j++)
                {
                    springs[j].node1 = &meshes[i].vertices[edge[j].firstVertex];
                    springs[j].node2 = &meshes[i].vertices[edge[j].secondVertex];                   
                }

                //Blending
                for (int j = 0; j < diagonal.size(); j++)
                {
                    springs[edge.size()-1 + j].node1 = &meshes[i].vertices[diagonal[j].firstVertex];
                    springs[edge.size() - 1 + j].node2 = &meshes[i].vertices[diagonal[j].secondVertex];
                    //springs.push_back(Spring(&(meshes[i].vertices[diagonal[j].firstVertex]), &(meshes[i].vertices[diagonal[j].secondVertex]), bendingCoef, dampCoef));
                }
            }
        }
        
    }

    void SimulateWind(glm::vec3 windDir) 
    { 
        for (int i = 0; i < meshes.size(); i++) 
        {
            for (int j = 0; j < meshes[i].vertices.size(); j++) 
            {
                meshes[i].vertices[j].force += (windDir * meshes[i].vertices[j].mass * 2.5f);
            }
        } 
    }

    void CollisionTest(glm::vec3 planeVertex,glm::vec3 normal) 
    {
        for (int i = 0; i < meshes.size(); i++) 
        {
            for (int j = 0; j < meshes[i].vertices.size(); j++) 
            {
                float distance = glm::length(meshes[i].vertices[j].Position - planeVertex);

                glm::vec3 normal = (meshes[i].vertices[j].Position - planeVertex) / distance;
                glm::vec3 Vn = glm::dot(meshes[i].vertices[j].velocity, normal) * normal;
                glm::vec3 Vt = meshes[i].vertices[j].velocity - normal;
                float a = max(float(1 - 0.3 * (1 + 0.2) * glm::length(Vn) / glm::length(Vt)), 0.0f);
                Vn = -1.0f * 0.2f * Vn;
                Vt = a * Vt;

                glm::vec3 Vnew = Vn + Vt;
                if (distance <= 1.55 && distance >= 0.025)
                {
                    meshes[i].vertices[j].velocity = Vnew;
                    meshes[i].vertices[j].velocity = glm::vec3(0);
                }
                else if (distance < 0.025)
                {
                    meshes[i].vertices[j].velocity = glm::vec3(0);
                    meshes[i].vertices[j].Position += 0.005f * normal;
                }
               
            }
        }
    }

    void CollisionBallTest(glm::vec3 center, float radian) 
    {
        for (int i = 0; i < meshes.size(); i++)
        {
            for (int j = 0; j < meshes[i].vertices.size(); j++)
            {
                float distance = glm::length(meshes[i].vertices[j].Position - center);
                glm::vec3 normal = (meshes[i].vertices[j].Position - center) / distance;
                glm::vec3 Vn = glm::dot(meshes[i].vertices[j].velocity, normal) * normal;
                glm::vec3 Vt = meshes[i].vertices[j].velocity - normal;
                float a = max(float(1 - 0.3 * (1 + 0.2) * glm::length(Vn) / glm::length(Vt)), 0.0f);
                Vn = -1.0f * 0.2f * Vn;
                Vt = a * Vt;

                glm::vec3 Vnew = Vn + Vt;

                if (distance < 0.775f && distance > 0.725f)
                {
                    meshes[i].vertices[j].velocity = Vnew;
                    isRotating = true;
                    meshes[i].vertices[j].Position += 0.005f * normal;
                }
                else if(distance <= 0.725f && distance>0.7f)
                {
                    meshes[i].vertices[j].velocity = Vnew;
                    isRotating = true;
                    meshes[i].vertices[j].Position += 0.005f * normal;
                }
            }
        }

    }

    void SimulateFriction(glm::vec3 center,float radian) 
    {
        for (int i = 0; i < meshes.size(); i++)
        {
            for (int j = 0; j < meshes[i].vertices.size(); j++) 
            {
                float distance = glm::length(meshes[i].vertices[j].Position - center);
                if (isRotating)
                {
                    glm::vec3 pressure = meshes[i].vertices[j].Position - center;
                    glm::vec3 pressureMagnitude = pressure / glm::length(pressure);

                    float coseta = glm::dot(pressureMagnitude, glm::vec3(0, -1, 0));
                    glm::vec3 target = gravity * coseta;

                    float maxStaticFriction = staticU * glm::length(target);

                    glm::vec3 temp_tagent = glm::cross(meshes[i].vertices[j].Position - center, gravity);

                    glm::vec3 tangent = temp_tagent / glm::length(temp_tagent);

                    float currentFriction = 1 * ballacc;
                    if (currentFriction > maxStaticFriction)
                    {
                        if (distance <7.5f)
                        {
                            float dynamicFiction = dynamicU * glm::length(target);
                            meshes[i].vertices[j].velocity += dynamicFiction * tangent * 7.5f;

                        }
                      
                    }

                }
                    
            }
        }
    }

    void SimulateCorners(int firstIndex, int secondIndex) 
    {
        for (int i = 0; i < meshes.size(); i++)
        {
            for (int j = 0; j < meshes[i].vertices.size(); j++) 
            {
                meshes[i].vertices[firstIndex].isFixed = true;
                meshes[i].vertices[secondIndex].isFixed = true;
            }
        }
    }

private:
    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(string const& path)
    {
        // read file via ASSIMP
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
        // check for errors
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }
        // retrieve the directory path of the filepath
        directory = path.substr(0, path.find_last_of('/'));
        // process ASSIMP's root node recursively
        processNode(scene->mRootNode, scene);
    }

    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode* node, const aiScene* scene)
    {
        // process each mesh located at the current node
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            // the node object only contains indices to index the actual objects in the scene. 
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }

    }

    Mesh processMesh(aiMesh* mesh, const aiScene* scene)
    {
        // data to fill
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        vector<Texture> textures;

        // walk through each of the mesh's vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
            // positions
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;
            // normals
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
                index_Normal.push_back(i);
            }
            // texture coordinates
            if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
            {
                glm::vec2 vec;
                // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
                // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
                index_Tex.push_back(i);
                // tangent
                vector.x = mesh->mTangents[i].x;
                vector.y = mesh->mTangents[i].y;
                vector.z = mesh->mTangents[i].z;
                vertex.Tangent = vector;
                // bitangent
                vector.x = mesh->mBitangents[i].x;
                vector.y = mesh->mBitangents[i].y;
                vector.z = mesh->mBitangents[i].z;
                vertex.Bitangent = vector;
            }
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);

            vertices.push_back(vertex);
        }
        // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            temp_faces.push_back(face);
            // retrieve all indices of the face and store them in the indices vector
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }
        // process materials
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        // 1. diffuse maps
        vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        // 2. specular maps
        vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        // 3. normal maps
        std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        // 4. height maps
        std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

        // return a mesh object created from the extracted mesh data
        return Mesh(vertices, indices, textures);
    }

    vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName)
    {
        vector<Texture> textures;
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
            bool skip = false;
            for (unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
                    break;
                }
            }
            if (!skip)
            {   // if texture hasn't been loaded already, load it
                Texture texture;
                texture.id = TextureFromFile(str.C_Str(), this->directory);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
            }
        }
        return textures;
    }
};

unsigned int TextureFromFile(const char* path, const string& directory, bool gamma)
{
    string filename = string(path);
    filename = directory + '/' + filename;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
#endif