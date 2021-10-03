#include <stdlib.h>
#include <iostream>
#include <cstdio>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include <chrono>

std::string g_filePath;

struct vec4
{
	float x, y, z, w;
};

struct vec2
{
	float u, v;
};
// For storing the mesh data - vertex, normal and texture coordinate information
struct OBJVertex {
	vec4 vertex;
	vec4 normal;
	vec2 uvCoord;
};

struct OBJMaterial
{
//private:
	std::string name;
	vec4		kA;			//Ambient Light Colour - alpha component stores Optical Density (Ni) (Refraction Index 0.001 - 10)
	vec4		kD;			//Difuse Light Colour - alpha component stores dissolve (d)(0-1)
	vec4		kS;			//Specular Light Colour - (exponent stored in alpha)

//public:
//	vec4 GetAmbient();
//	void SetAmbient(vec4);
//	vec4 GetDifuse();
//	void SetDiffuse(vec4);
//	vec4 GetSpecular();
//	void SetSpecualar(vec4);

};

bool ProcessLine(const std::string&, std::string&, std::string&);
vec4 processVectorString(const std::string);
std::vector<std::string> splitStringAtCharacter(std::string, char);
OBJVertex processFaceData(std::string, std::vector<vec4>&, std::vector<vec4>&, std::vector<vec2>&);
vec4 processVectorString(const std::string);
void LoadMaterialLibrary(std::string, std::vector<OBJMaterial>&);



int main(int argc, char* argv[])
{
	// Opening the obj file for testing
	std::string filename;
	std::cout << "Please enter the obj file you would like to open" << std::endl;
	std::cin >> filename;
	std::cout << "Attempting to open file: " << filename << std::endl;
	// use fstream to read file data 
	std::fstream file;
	file.open("obj_models/" + filename, std::ios_base::in | std::ios_base::binary);

	if (file.is_open())
	{
		// Get File Path Information
		g_filePath = filename;
		size_t path_end = g_filePath.find_first_of("/\\");
		if (path_end != std::string::npos)
		{
			g_filePath = g_filePath.substr(0, path_end + 1);
		}
		else
		{
			g_filePath = "";
		}
		std::cout << "Successfully opened!" << std::endl;
		// Displaying the file size
		// Attempt to read the maximum number of bytes available from the file
		file.ignore(std::numeric_limits<std::streamsize>::max());
		// The fstream gcounter will now be at the end of the file, gcount is a byte offset from 0
		// 0 is the start of the file, or how many bytes file.ignore just read
		std::streamsize fileSize = file.gcount();
		//Clear the EOF marker from being read
		file.gcount();
		//reset the seekg back to the start of the file
		file.seekg(0, std::ios_base::beg);
		//Write out the files size to console if it contains data
		if (fileSize != 0)
		{
			std::cout << std::fixed;
			std::cout << std::setprecision(2);
			std::cout << "File Size: " << fileSize / 1024 << "KB" << std::endl;
			// File is open adn contains data 
			// Read each line of the file and display to the console
			std::string fileLine;
			// while the end of file (EOF) token has not been read
			
			// While the end of file (EOF) token has not been read
			std::map< std::string, int32_t > faceIndexMap;
			std::vector<vec4> vertexData;
			std::vector<vec4> normalData;
			std::vector<vec2> textureData;
			std::vector<OBJVertex> meshData;
			std::vector<uint32_t> meshIndices;
			std::vector<OBJMaterial> materials;
			
			while (!file.eof())
			{
				if (std::getline(file, fileLine))
				{
					// If we get here we managed to read a line from the file
					std::string key;
					std::string value;
					// Extracting the data 
					if (ProcessLine(fileLine, key, value))
					{
						if (key == "#") // then this is a comment line
						{
							std::cout << value << std::endl;
						}
						if (key == "mtllib")
						{
							std::cout << "Material File: " << value << std::endl;
							//Load in Material file so that materials can be used as required
							LoadMaterialLibrary(value, materials);
							continue;
						}

						if (key == "v")
						{
							vec4 vertex = processVectorString(value);
							vertex.w = 1.f; // as this is positional data ensure that w component is set to 1
							vertexData.push_back(vertex);

						}
						if (key == "vn")
						{
							vec4 normal = processVectorString(value);
							normal.w = 0.f; // as this is directional data ensure that w component is set to 0
							normalData.push_back(normal);
						}
						if (key == "vt")
						{
							vec4 vec = processVectorString(value);
							vec2 uvCoord = { vec.x , vec.y };
							textureData.push_back(uvCoord);
						}
						if (key == "f") // Face Data - 3 or more vertices for a triangle or fan of triangles
						{
							// Splitting the data within the string at the space character to produce a vector of face triplets -
							// v/vt/vn - vertex, vertex texture and the vertex normal
							std::vector<std::string> faceComponents = splitStringAtCharacter(value, ' ');
							std::vector<uint32_t> faceIndices;

							for (auto iter = faceComponents.begin(); iter != faceComponents.end(); ++iter)
							{
								// See if the face has already been processed
								auto searchKey = faceIndexMap.find(*iter);
								if (searchKey != faceIndexMap.end())
								{
									// We have already processed this vertex
									//std::cout << "Processing repeat face data: " << (*iter) << std::endl;
									faceIndices.push_back((*searchKey).second);
								}
								else
								{
									OBJVertex vertex = processFaceData(*iter, vertexData, normalData, textureData);
									meshData.push_back(vertex);
									uint32_t index = ((uint32_t)meshData.size() - 1);
									faceIndexMap[*iter] = index;
									faceIndices.push_back(index);
								}
								// now that all triplets have been processed process indexBuffer
								for (int i = 1; i < faceIndices.size() - 1; i++)
								{
									meshIndices.push_back(faceIndices[0]);
									meshIndices.push_back(faceIndices[i]);
									meshIndices.push_back(faceIndices[(size_t)i + 1]);
								}
							}
						}
					}
				}
			}
			// once the face data has been processed and logged to the console, we can empty the vertex/normal/texture arrays
			vertexData.clear();
			normalData.clear();
			textureData.clear();
			std::cout << "Processed " << meshData.size() << " verticies in OBJ file" << std::endl;
		}
		else
		{
			std::cout << "File contains no data, closing file" << std::endl;
		}
		file.close();
	}
	else
	{
		std::cout << "Unable to open file: " << filename << std::endl;
	}
	return EXIT_SUCCESS;
}
//\------------------------
//\ PROCESS LINE
//\------------------------

bool ProcessLine(const std::string& a_inLine, std::string& a_outKey, std::string& a_outValue)
{
	if (!a_inLine.empty())
	{
		// find the first character on the line that is not a space or a tab
		size_t keyStart = a_inLine.find_first_not_of(" \t\r\n");
		// If key start is not valid 
		if (keyStart == std::string::npos)
			return false;
		// Starting from the key find the first character that is not a space or a tab
		size_t keyEnd = a_inLine.find_first_of(" \t\r\n", keyStart);
		size_t valueStart = a_inLine.find_first_not_of(" \t\r\n", keyEnd);
		// find the end postition for the value
		// from the end of the line find the last character that isn't a space , tab, newline or return
		// +1 to include the last character itself
		size_t valueEnd = a_inLine.find_last_not_of(" \t\r\n") + 1;
		// now that we have the start and end positions for the data use substring
		a_outKey = a_inLine.substr(keyStart, keyEnd - keyStart);
		if (valueStart == std::string::npos) // npos means until the end of the string - so if the start is the eng ignore it.
		{
			// If we get here then we had a line with a key and no data value
			//E.g. "# \n"
			a_outValue = "";
			return true;
		}
		a_outValue = a_inLine.substr(valueStart, valueEnd - valueStart);
		return true;
	}
	return false;
}

//\------------------------
//  SPLITTING THE STRING AT THE CHARACTER
//\------------------------

// A function that splits a string at a given character into an array e.g. splitting faces into their face triplets with the space
// character and to split those triplets into seperate indicies with the '/' character
std::vector<std::string> splitStringAtCharacter(std::string data, char a_character)
{
	std::vector<std::string> lineData;
	std::stringstream iss(data);
	std::string lineSegment;
	// provinding a character to the getline  function splits the line at occurances of that character
	while (std::getline(iss, lineSegment, a_character))
	{
		// Push each line segment into a vector
		lineData.push_back(lineSegment);
	}
	return lineData;
}

//\------------------------
//  PROCESSING THE FACE DATA
//\------------------------

// This function splits the face triplet data at the '/' character and gets bthe index into the vertex/normal/texture data 
// arrays from this triple to produce the OBJVertex made up of that vector data.
OBJVertex processFaceData(std::string a_faceData, std::vector<vec4>& a_vertexArray,
	std::vector<vec4>& a_normalArray, std::vector<vec2>& a_uvArray)
{
	std::vector<std::string> vertexIndices = splitStringAtCharacter(a_faceData, '/');
	// a simple local structure to hold face triplet data as integer values
	typedef struct objectFaceTriplet { int32_t v, vn, vt; }; objectFaceTriplet;
	// instance of objFaceTriplet struct
	objectFaceTriplet ft = { 0, 0, 0 };
	// parse vertex indices as integer values to look up in vertex/normal/uv array data
	ft.v = std::stoi(vertexIndices[0]);
	// If ise is >= 2 then there is additional information outside of vertex data
	if (vertexIndices.size() >= 2)
	{
		// if size of element 1 is greater than 0 then UV - COORD information present
		if (vertexIndices[1].size() > 0)
		{
			ft.vt = std::stoi(vertexIndices[1]);
		}
		// if size is greater than3 then there is normal data present
		if (vertexIndices.size() >= 3)
		{
			ft.vn = std::stoi(vertexIndices[2]);
		}
	}
	// now that face index values have been processed retrieve actual data from vertex arrays
	OBJVertex currentVertex;
	currentVertex.vertex = a_vertexArray[size_t(ft.v) - 1];
	if (ft.vn != 0)
	{
		currentVertex.normal = a_normalArray[size_t(ft.vn) - 1];
	}
	if (ft.vt != 0)
	{
		currentVertex.uvCoord = a_uvArray[size_t(ft.vt) - 1];
	}
	return currentVertex;
}

//\------------------------
//\ PROCESS VECTOR STRING
//\------------------------

//This function uses the STL stringstream class to manipulate the string that is passed in. Using the ">>" operator to
//move through the string and capture the floating point data into a cariable using the STL string to float function (stof)
vec4 processVectorString(const std::string a_data)
{
	//splirt the line data at eash sapce character and store this as a float value within a glm::vec4
	std::stringstream iss(a_data);
	// create a zero vec4
	vec4 vecData = { 0.f, 0.f, 0.f, 0.f };
	int i = 0;
	// For loop to loop until iss cannot stream data into val
	for (std::string val; iss >> val; ++i)
	{
		// use std::string to float function
		float fVal = std::stof(val);
		// cast vec4 to float* to allow iteration through elements of vec4 - casting to get the correct index
		((float*)(&vecData))[i] = fVal;
	}
	return vecData;
}

//\------------------------
//\ LOAD MATERIAL LIBRARY
//\------------------------

void LoadMaterialLibrary(std::string a_mtllib, std::vector<OBJMaterial>& a_materials)
{
	std::string matFile = g_filePath + a_mtllib;
	std::cout << "Attempting to load Materials file: " << matFile << std::endl;
	// get sn fstream to read in the file data
	std::fstream file;
	file.open(matFile, std::ios_base::in | std::ios_base::binary);
	// Test to see if the file has opened correctly
	if (file.is_open())
	{
		std::cout << "Material Library Successfully opened" << std::endl;
		// Success file has been opened - Now we need to check the file contains info and isn't empty
		file.ignore(std::numeric_limits<std::streamsize>::max());	// attempts to read the highest number of bytes from the file
		std::streamsize fileSize = file.gcount();					// gCount will have reached an EOF marker, letting us know the byte size
		file.clear();												// clear EOF marker from being read
		file.seekg(0, std::ios_base::beg);							// seek back to the beginning of the file
		if (fileSize == 0)											// Close the file and return if it's empty
		{
			std::cout << "File contains no data, closing file" << std::endl;
			file.close();
		}
		std::cout << "Material File Size: " << fileSize / 1024 << " KB" << std::endl;

		// Variable to store file data as it is read line by line
		std::string fileLine;
		// Pointer to a masterial object - (The one at the end of the materials vector)
		OBJMaterial* currentMaterial = nullptr;

		while (!file.eof())
		{
			if (std::getline(file, fileLine))
			{
				std::string key;
				std::string value;
				if (ProcessLine(fileLine, key, value))
				{
					if (key == "#")
					{
						std::cout << value << std::endl;
						continue;
					}
					if (key == "newmtl")
					{
						std::cout << "New material found: " << value << std::endl;
						a_materials.push_back(OBJMaterial());
						currentMaterial = &(a_materials.back());
						currentMaterial->name = value;
						continue;
					}
					if (key == "Ns")
					{
						if (currentMaterial != nullptr)
						{
							// Ns is guaranteed to be a single float value
							currentMaterial->kS.w = std::stof(value);
						}
						continue;
					}
					if (key == "Ka")
					{
						if (currentMaterial != nullptr)
						{
							// Process kA as vector string
							float kAd = currentMaterial->kA.w; // Store alpha channel as may contrain refractive index
							currentMaterial->kA = processVectorString(value);
							currentMaterial->kA.w = kAd;
						}
						continue;
					}
					if (key == "Kd")
					{
						if (currentMaterial != nullptr)
						{
							// Process kD as a vector string
							float kDa = currentMaterial->kD.w; // Store alpha as may contain dissolve value
							currentMaterial->kD = processVectorString(value);
							currentMaterial->kD.w = kDa;
						}
						continue;
					}
					if (key == "Ks")
					{
						if (currentMaterial != nullptr)
						{
							// Process Ks as vector string
							float kSa = currentMaterial->kS.w; // Store alpha as may contain specular component
							currentMaterial->kS = processVectorString(value);
							currentMaterial->kS.w = kSa;
						}
						continue;
					}
					if (key == "Ke")
					{
						// KE is for emissive properties - we will NOT need to support this for our pruposes
						continue;
					}
					if (key == "Ni")
					{
						if (currentMaterial != nullptr)
						{
							// this is the refractive index of the mesh (How light bends as it passes through materials
							// we will store this in the alpha component of the ambient light values (kA)
							currentMaterial->kA.w = std::stof(value);
						}
						continue;
					}
					if (key == "d" || key == "Tr") // Transparency/Opacity Tr = 1 - d
					{
						if (currentMaterial != nullptr)
						{
							// This is the dissolve or alpha value of the material, we will store this in the kD alpha channel
							currentMaterial->kD.w = std::stof(value);
							if (key == "Tr")
							{
								currentMaterial->kD.w = 1.f - currentMaterial->kD.w;
							}
						}
						continue;
					}
					if (key == "illum")
					{
						// illum describes the illumination model used to light the model.
						// Ignore this for now as we will light the scene our own way
						continue;
					}
				}
			}
		}
		file.close();
	}
}