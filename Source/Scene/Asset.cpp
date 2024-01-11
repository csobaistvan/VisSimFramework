////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Asset.h"
#include "Scene/Includes.h"

// ImGui custom glyphs
#include "imgui_plugins/CustomFontBody.h"

namespace Asset
{
	////////////////////////////////////////////////////////////////////////////////
	TextFile::TextFile() :
		TextFile(std::string())
	{}

	////////////////////////////////////////////////////////////////////////////////
	TextFile::TextFile(std::string const& contents) :
		m_contents(contents)
	{}

	////////////////////////////////////////////////////////////////////////////////
	TextFile::operator std::string() const
	{
		return m_contents;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool makeDirectoryStructure(std::string const& fileName)
	{
		return std::filesystem::create_directories(std::filesystem::path(fileName).parent_path());
	}

	////////////////////////////////////////////////////////////////////////////////
	bool existsFile(std::filesystem::path const& filePath)
	{
		return std::filesystem::exists(filePath);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool existsFile(std::string const& fileName)
	{
		return std::filesystem::exists(std::filesystem::path(fileName));
	}

	////////////////////////////////////////////////////////////////////////////////
	void storeTextFile(Scene::Scene& scene, const std::string& fileName, std::string const& contents)
	{
		scene.m_textFiles[fileName] = contents;
	}

	////////////////////////////////////////////////////////////////////////////////
	std::optional<TextFile> loadTextFile(Scene::Scene& scene, const std::string& fileName, bool allowCaching)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, fileName);

		// Path to the file
		std::filesystem::path filePath = EnginePaths::assetsFolder() / fileName;

		// Get the last write time of the file
		std::optional<std::filesystem::file_time_type> lastWrite;
		if (std::filesystem::exists(filePath))
			lastWrite = std::filesystem::last_write_time(filePath);

		// Make sure it isn't loaded already.
		auto it = scene.m_textFiles.find(fileName);
		if (allowCaching && it != scene.m_textFiles.end() && (lastWrite.has_value() == false || (lastWrite.has_value() && it->second.m_lastModified >= lastWrite)))
			return it->second;

		Debug::log_trace() << "Loading text file: " << fileName << Debug::end;

		// Create the stream
		std::ifstream fstream(EnginePaths::assetsFolder() / fileName);
		if (!fstream.good())
		{
			if (it != scene.m_textFiles.end()) return it->second;

			Debug::log_error() << "Unable to load text file: " << fileName << Debug::end;
			return std::nullopt;
		}

		// Load the text file
		std::stringstream inputStream;
		inputStream << fstream.rdbuf();

		// Store it
		scene.m_textFiles[fileName].m_contents = inputStream.str();
		scene.m_textFiles[fileName].m_lastModified = std::filesystem::last_write_time(filePath);

		Debug::log_trace() << "Successfully loaded text file: " << fileName << Debug::end;

		return scene.m_textFiles[fileName];
	}

	////////////////////////////////////////////////////////////////////////////////
	bool saveTextFile(Scene::Scene& scene, const std::string& fileName, const std::string& contents)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, fileName);

		Debug::log_trace() << "Saving text file: " << fileName << Debug::end;

		// Full file path
		std::string fullPath = (EnginePaths::assetsFolder() / fileName).string();

		// Create the parent folder hierarchy
		EnginePaths::makeDirectoryStructure(fullPath, true);

		// Save the text file
		std::ofstream fstream(fullPath);
		if (fstream.good()) fstream << contents;

		// Store it
		scene.m_textFiles[fileName] = contents;

		Debug::log_trace() << "Successfully saved text file: " << fileName << Debug::end;
		
		return true;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool saveTextFile(Scene::Scene& scene, const std::string& fileName, const std::stringstream& contents)
	{
		return saveTextFile(scene, fileName, contents.str());
	}

	////////////////////////////////////////////////////////////////////////////////
	std::optional<std::ordered_pairs_in_blocks> loadPairsInBlocks(Scene::Scene& scene, const std::string& fileName)
	{
		Debug::log_trace() << "Loading pairs-in-blocks from file: " << fileName << Debug::end;

		// Load the raw text file
		auto textContents = loadTextFile(scene, fileName);
		if (textContents.has_value() == false)
		{
			Debug::log_error() << "Unable to load pairs-in-blocks from file: " << fileName << ", reason: unable to open file." << Debug::end;

			return std::optional<std::ordered_pairs_in_blocks>();
		}

		// Make an input stream from the contents
		std::istringstream iss(textContents.value());

		// Result of the load
		std::ordered_pairs_in_blocks result;

		// Contents of the current category
		std::ordered_pairs_in_blocks::iterator currentCategory;

		// Regex used for the search
		static std::regex s_headerRegex = std::regex("[[:space:]]*\\[(.*?)\\][[:space:]]*");
		static std::regex s_contentRegex = std::regex("(.*?)[[:space:]]*=[[:space:]]*(.*)");

		// Process each line
		for (std::string line; std::getline(iss, line);)
		{
			// Try to match a header
			std::smatch capture;
			if (std::regex_match(line, capture, s_headerRegex))
			{
				// Append a new category and set it as the current one
				result[capture[1]] = {};
				currentCategory = result.find(capture[1]);

				//Debug::log_debug() << "Found category \"" << capture[1] << "\"" << Debug::end;
			}

			// Try to match a content regex
			else if (std::regex_search(line, capture, s_contentRegex))
			{
				// Append the value
				currentCategory->second.push_back(std::make_pair(capture[1], capture[2]));

				//Debug::log_debug() << "Adding to \"" << currentCategory->first << "\" row: \"" << capture[1] << "\"" << Debug::end;
			}
		}

		Debug::log_trace() << "Successfully loaded pairs-in-blocks to file: " << fileName << Debug::end;

		// Return the result
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool savePairsInBlocks(Scene::Scene& scene, const std::string& fileName, const std::ordered_pairs_in_blocks& pairs, bool alignCols)
	{
		Debug::log_trace() << "Saving pairs-in-blocks to file: " << fileName << Debug::end;

		std::stringstream result;

		// serialize the entries
		for (auto const& entryType : pairs)
		{
			// header
			result << "[" << entryType.first << "]" << std::endl;

			// first column width
			size_t firstColWidth, secondColWidth;
			if (alignCols)
			{
				firstColWidth = std::max_element(entryType.second.begin(), entryType.second.end(), 
					[](auto const& a, auto const& b) { return a.first.length() < b.first.length(); })->first.length();
				secondColWidth = std::max_element(entryType.second.begin(), entryType.second.end(), 
					[](auto const& a, auto const& b) { return a.second.length() < b.second.length(); })->second.length();
			}
			
			for (auto const& entry : entryType.second)
			{
				if (alignCols)
				{
					result 
						<< std::left << std::setw(firstColWidth) << entry.first 
						<< " = " 
						<< std::right << std::setw(secondColWidth) << entry.second 
						<< std::endl;
				}
				else
				{
					result 
						<< entry.first 
						<< " = " 
						<< entry.second 
						<< std::endl;
				}
			}

			// separator
			result << std::endl;
		}

		Debug::log_trace() << "Successfully saved pairs-in-blocks to file: " << fileName << Debug::end;

		return saveTextFile(scene, fileName, result);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool savePairsInBlocks(Scene::Scene& scene, const std::string& fileName, const std::unordered_pairs_in_blocks& pairs, bool alignCols)
	{
		std::ordered_pairs_in_blocks ordered;

		// convert it to an ordered list
		for (auto const& entryType : pairs)
		{
			ordered[entryType.first].reserve(entryType.second.size());
			for (auto const& entry : entryType.second)
			{
				ordered[entryType.first].push_back({ entry.first, entry.second });
			}
		}

		return savePairsInBlocks(scene, fileName, ordered);
	}

	////////////////////////////////////////////////////////////////////////////////
	bool saveCsv(Scene::Scene& scene, std::string const& fileName, std::vector<std::vector<std::string>> const& contents, std::string const& separator, std::vector<std::string> const& headers)
	{
		std::stringstream result;

		// Helper function for writing out a line
		auto writeCsvLine = [&separator](std::stringstream& result, std::vector<std::string> const& line)
		{
			for (size_t i = 0; i < line.size(); ++i)
			{
				if (i > 0) result << separator;
				result << line[i];
			}
		};

		// Print out the headers, if any
		if (headers.empty() == false)
		{
			writeCsvLine(result, headers);
			result << std::endl;
		}

		// Print out the lines
		for (auto const& line : contents)
		{
			writeCsvLine(result, line);
			result << std::endl;
		}

		return saveTextFile(scene, fileName, result);
	}

	////////////////////////////////////////////////////////////////////////////////
	std::optional<cv::Mat> loadImage(Scene::Scene& scene, const std::string& filePath)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, filePath);

		Debug::log_trace() << "Loading image: " << filePath << Debug::end;

		// Compute the full file name
		std::string fullFileName = (EnginePaths::assetsFolder() / filePath).string();

		// Try to load the image.
		int width, height, components;
		stbi_set_flip_vertically_on_load(1);
		unsigned char* image = stbi_load(fullFileName.c_str(), &width, &height, &components, 4);

		// Make sure it was successful.
		if (image == nullptr)
		{
			Debug::log_error() << "Unable to load texture: " << filePath << Debug::end;
			return std::optional<cv::Mat>();
		}
		
		// Result
		cv::Mat result(height, width, CV_MAKETYPE(CV_8U, 4));
		for (int x = 0; x < width; ++x)
		for (int y = 0; y < height; ++y)
		{
			size_t arrayId = (y * width + x) * 4;

			result.at<cv::Vec4b>(y, x) = cv::Vec4b(image[arrayId + 0], image[arrayId + 1], image[arrayId + 2], image[arrayId + 3]);
		}

		Debug::log_trace() << "Image dimensions: " << width << ", " << height << Debug::end;
		Debug::log_trace() << "cv::Mat dimensions: " << result.cols << ", " << result.cols << Debug::end;

		// Free the image data.
		stbi_image_free(image);

		Debug::log_trace() << "Successfully loaded image: " << filePath << Debug::end;

		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool loadTexture(Scene::Scene& scene, const std::string& textureName, const std::string& filePath)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, filePath);

		// Make sure it isn't loaded already.
		if (scene.m_textures.find(textureName) != scene.m_textures.end())
			return true;

		Debug::log_trace() << "Loading texture: '" << filePath << "'" << Debug::end;

		// Compute the full file name
		std::string fullFileName = (EnginePaths::assetsFolder() / filePath).string();

		// Try to load the image.
		int width, height, components;
		stbi_set_flip_vertically_on_load(1);
		unsigned char* image = stbi_load(fullFileName.c_str(), &width, &height, &components, 4);

		// Make sure it was successful.
		if (image == nullptr)
		{
			Debug::log_error() << "Unable to load texture: '" << filePath << "'" << Debug::end;
			return false;
		}

		// Guess the format from the number of components
		GLenum format = GL_RGBA8;
		GLenum layout = GL_RGBA;
		/*
		GLenum format, layout;
		if (components == 1)
		{
			format = GL_R8;
			layout = GL_RED;
		}
		else if (components == 2)
		{
			format = GL_RG8;
			layout = GL_RG;
		}
		else if (components == 3)
		{
			format = GL_RGB8;
			layout = GL_RGB;
		}
		else if (components == 4)
		{
			format = GL_RGBA8;
			layout = GL_RGBA;
		}
		*/

		// The created texture object.
		GPU::Texture texture;

		// Store the texture dimensions.
		texture.m_type = GL_TEXTURE_2D;
		texture.m_width = width;
		texture.m_height = height;
		texture.m_depth = 1;
		texture.m_numDimensions = 2;
		texture.m_dimensions = glm::ivec3(width, height, 1);
		texture.m_format = format;
		texture.m_layout = layout;
		texture.m_minFilter = GL_LINEAR_MIPMAP_LINEAR;
		texture.m_magFilter = GL_LINEAR;
		texture.m_wrapMode = GL_REPEAT;
		texture.m_anisotropy = 16.0f;
		texture.m_mipmapped = true;

		// Upload the texture data.
		glGenTextures(1, &texture.m_texture);
		glBindTexture(texture.m_type, texture.m_texture);
		glTexImage2D(texture.m_type, 0, texture.m_format, texture.m_width, texture.m_height, 0, texture.m_layout, GL_UNSIGNED_BYTE, image);
		glTexParameteri(texture.m_type, GL_TEXTURE_WRAP_S, texture.m_wrapMode);
		glTexParameteri(texture.m_type, GL_TEXTURE_WRAP_T, texture.m_wrapMode);
		glTexParameteri(texture.m_type, GL_TEXTURE_MIN_FILTER, texture.m_minFilter);
		glTexParameteri(texture.m_type, GL_TEXTURE_MAG_FILTER, texture.m_magFilter);
		glTexParameterf(texture.m_type, GL_TEXTURE_MAX_ANISOTROPY_EXT, texture.m_anisotropy);
		glGenerateMipmap(texture.m_type);
		glBindTexture(texture.m_type, 0);

		// Associate the proper label to it
		glObjectLabel(GL_TEXTURE, texture.m_texture, textureName.length(), textureName.c_str());
		
		// Free the image data.
		stbi_image_free(image);

		// Store the texture.
		scene.m_textures[textureName] = texture;

		Debug::log_trace() << "Successfully loaded texture: " << filePath << Debug::end;

		return true;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool load3DTexture(Scene::Scene& scene, const std::string& textureName, const std::string& filePath)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, filePath);

		// Make sure it isn't loaded already.
		if (scene.m_textures.find(textureName) != scene.m_textures.end())
			return true;

		Debug::log_trace() << "Loading 3D texture: " << filePath << Debug::end;

		// Compute the full file name
		std::string fullFileName = (EnginePaths::assetsFolder() / filePath).string();

		// Try to load the image.
		int width, height, components;
		stbi_set_flip_vertically_on_load(1);
		unsigned char* image = stbi_load(fullFileName.c_str(), &width, &height, &components, 4);

		// Make sure it was successful.
		if (image == nullptr)
		{
			Debug::log_error() << "Unable to load 3D texture: " << filePath << Debug::end;
			return false;
		}
		
		// Compute the dimensions of each layer
		int columns, rows, layers;
		if (width > height)
		{
			layers = columns = width / height;
			rows = 1;
		}
		else
		{
			layers = rows = height / width;
			columns = 1;
		}
		unsigned int layerWidth = width / columns;
		unsigned int layerHeight = height / rows;

		// The created texture object.
		GPU::Texture texture;

		// Store the texture dimensions.
		texture.m_type = GL_TEXTURE_3D;
		texture.m_width = layerWidth;
		texture.m_height = layerHeight;
		texture.m_depth = layers;
		texture.m_numDimensions = 3;
		texture.m_dimensions = glm::ivec3(layerWidth, layerHeight, layers);
		texture.m_format = GL_RGBA8;
		texture.m_layout = GL_RGBA;
		texture.m_minFilter = GL_LINEAR_MIPMAP_LINEAR;
		texture.m_magFilter = GL_LINEAR;
		texture.m_wrapMode = GL_REPEAT;
		texture.m_anisotropy = 0.0f;
		texture.m_mipmapped = false;

		// Rearrange the pixel data
		std::unique_ptr<unsigned char[]> pixels(new unsigned char[width * height * 4]);

		for (unsigned layerCol = 0; layerCol < columns; ++layerCol)
		for (unsigned layerRow = 0; layerRow < rows; ++layerRow)
		{
			for (unsigned h = 0; h < layerHeight; ++h)
			for (unsigned w = 0; w < layerWidth; ++w)
			{
				unsigned srcId = 4 * ((layerRow * layerHeight + h) * width + (layerCol * layerWidth + w));
				unsigned dstId = 4 * ((layerRow * columns + layerCol) * (layerWidth * layerHeight) + h * layerWidth + w);

				for (size_t k = 0; k < 4; ++k)
					pixels[dstId + k] = image[srcId + k];
			}
		}

		// Upload the texture data
		glGenTextures(1, &texture.m_texture);
		glBindTexture(texture.m_type, texture.m_texture);
		glTexImage3D(texture.m_type, 0, texture.m_format, texture.m_width, texture.m_height, texture.m_depth, 0, texture.m_layout, GL_UNSIGNED_BYTE, pixels.get());
		glTexParameteri(texture.m_type, GL_TEXTURE_WRAP_S, texture.m_wrapMode);
		glTexParameteri(texture.m_type, GL_TEXTURE_WRAP_T, texture.m_wrapMode);
		glTexParameteri(texture.m_type, GL_TEXTURE_WRAP_R, texture.m_wrapMode);
		glTexParameteri(texture.m_type, GL_TEXTURE_MIN_FILTER, texture.m_minFilter);
		glTexParameteri(texture.m_type, GL_TEXTURE_MAG_FILTER, texture.m_magFilter);
		glTexParameterf(texture.m_type, GL_TEXTURE_MAX_ANISOTROPY_EXT, texture.m_anisotropy);
		glGenerateMipmap(texture.m_type);
		glBindTexture(texture.m_type, 0);

		// Associate the proper label to it
		glObjectLabel(GL_TEXTURE, texture.m_texture, textureName.length(), textureName.c_str());

		// Free the image data.
		stbi_image_free(image);

		// Store the texture.
		scene.m_textures[textureName] = texture;

		Debug::log_trace() << "Successfully loaded 3D texture: " << filePath << Debug::end;

		return true;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool loadCubeMap(Scene::Scene& scene, const std::string& textureName, const std::string& filePath, const std::string& leftName, const std::string& rightName,
		const std::string& topName, const std::string& bottomName, const std::string& backName, const std::string& frontName)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, filePath);

		// Make sure it isn't loaded already.
		if (scene.m_textures.find(textureName) != scene.m_textures.end())
			return true;

		Debug::log_trace() << "Loading cubemap: " << filePath << Debug::end;

		// Generate the file paths
		std::string leftPath = (EnginePaths::assetsFolder() / filePath / leftName).string();
		std::string rightPath = (EnginePaths::assetsFolder() / filePath / rightName).string();
		std::string topPath = (EnginePaths::assetsFolder() / filePath / topName).string();
		std::string bottomPath = (EnginePaths::assetsFolder() / filePath / bottomName).string();
		std::string backPath = (EnginePaths::assetsFolder() / filePath / backName).string();
		std::string frontPath = (EnginePaths::assetsFolder() / filePath / frontName).string();

		// Try to load the image.
		int width, height, components;
		stbi_set_flip_vertically_on_load(1);

		unsigned char *left, *right, *bottom, *top, *front, *back;
		
		{
			Profiler::ScopedCpuPerfCounter perfCounter(scene, leftPath);

			left = stbi_load(leftPath.c_str(), &width, &height, &components, 4);

			if (left == nullptr)
			{
				Debug::log_error() << "Unable to load cubemap texture: " << leftPath << Debug::end;
				return false;
			}
		}

		{
			Profiler::ScopedCpuPerfCounter perfCounter(scene, rightPath);

			right = stbi_load(rightPath.c_str(), &width, &height, &components, 4);

			if (right == nullptr)
			{
				Debug::log_error() << "Unable to load cubemap texture: " << rightPath << Debug::end;
				return false;
			}
		}

		{
			Profiler::ScopedCpuPerfCounter perfCounter(scene, topPath);

			top = stbi_load(topPath.c_str(), &width, &height, &components, 4);

			if (top == nullptr)
			{
				Debug::log_error() << "Unable to load cubemap texture: " << topPath << Debug::end;
				return false;
			}
		}

		{
			Profiler::ScopedCpuPerfCounter perfCounter(scene, bottomPath);

			bottom = stbi_load(bottomPath.c_str(), &width, &height, &components, 4);

			if (bottom == nullptr)
			{
				Debug::log_error() << "Unable to load cubemap texture: " << bottomPath << Debug::end;
				return false;
			}
		}

		{
			Profiler::ScopedCpuPerfCounter perfCounter(scene, backPath);

			back = stbi_load(backPath.c_str(), &width, &height, &components, 4);

			if (back == nullptr)
			{
				Debug::log_error() << "Unable to load cubemap texture: " << backPath << Debug::end;
				return false;
			}
		}

		{
			Profiler::ScopedCpuPerfCounter perfCounter(scene, frontPath);

			front = stbi_load(frontPath.c_str(), &width, &height, &components, 4);

			if (front == nullptr)
			{
				Debug::log_error() << "Unable to load cubemap texture: " << frontPath << Debug::end;
				return false;
			}
		}

		// The created texture object.
		GPU::Texture texture;

		// Store the texture dimensions.
		texture.m_type = GL_TEXTURE_CUBE_MAP;
		texture.m_width = width;
		texture.m_height = height;
		texture.m_depth = 1;
		texture.m_numDimensions = 2;
		texture.m_dimensions = glm::ivec3(width, height, 1);
		texture.m_format = GL_RGBA8;
		texture.m_layout = GL_RGBA;
		texture.m_minFilter = GL_LINEAR_MIPMAP_LINEAR;
		texture.m_magFilter = GL_LINEAR;
		texture.m_wrapMode = GL_CLAMP_TO_EDGE;
		texture.m_anisotropy = 16.0f;
		texture.m_mipmapped = true;

		// Upload the texture data.
		glGenTextures(1, &texture.m_texture);
		glBindTexture(texture.m_type, texture.m_texture);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, texture.m_format, texture.m_width, texture.m_height, 0, texture.m_layout, GL_UNSIGNED_BYTE, right);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, texture.m_format, texture.m_width, texture.m_height, 0, texture.m_layout, GL_UNSIGNED_BYTE, left);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, texture.m_format, texture.m_width, texture.m_height, 0, texture.m_layout, GL_UNSIGNED_BYTE, bottom);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, texture.m_format, texture.m_width, texture.m_height, 0, texture.m_layout, GL_UNSIGNED_BYTE, top);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, texture.m_format, texture.m_width, texture.m_height, 0, texture.m_layout, GL_UNSIGNED_BYTE, front);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, texture.m_format, texture.m_width, texture.m_height, 0, texture.m_layout, GL_UNSIGNED_BYTE, back);
		glTexParameteri(texture.m_type, GL_TEXTURE_WRAP_S, texture.m_wrapMode);
		glTexParameteri(texture.m_type, GL_TEXTURE_WRAP_T, texture.m_wrapMode);
		glTexParameteri(texture.m_type, GL_TEXTURE_MIN_FILTER, texture.m_minFilter);
		glTexParameteri(texture.m_type, GL_TEXTURE_MAG_FILTER, texture.m_magFilter);
		glTexParameterf(texture.m_type, GL_TEXTURE_MAX_ANISOTROPY_EXT, texture.m_anisotropy);
		glGenerateMipmap(texture.m_type);
		glBindTexture(texture.m_type, 0);

		// Associate the proper label to it
		glObjectLabel(GL_TEXTURE, texture.m_texture, textureName.length(), textureName.c_str());

		// Free the image data.
		stbi_image_free(left);
		stbi_image_free(right);
		stbi_image_free(top);
		stbi_image_free(bottom);
		stbi_image_free(back);
		stbi_image_free(front);

		// Store the texture.
		scene.m_textures[textureName] = texture;

		Debug::log_trace() << "Successfully loaded cubemap: " << filePath << Debug::end;

		return true;
	}

	////////////////////////////////////////////////////////////////////////////////
	float linearizeDepth(float depth, float near, float far)
	{
		return (2.0f * near) / (far + near - depth * (far - near));
	}

	////////////////////////////////////////////////////////////////////////////////
	float reconstructCameraZ(float depth, glm::mat4 projection)
	{
		return projection[3][2] / (depth * -2.0 + 1.0 - projection[2][2]);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	void extractSubImage(T* pixels, int width, int height, int subWidth, int subHeight)
	{
		for (int y = 0; y < subHeight; ++y)
		{
			for (int x = 0; x < subWidth; ++x)
			{
				const int offset = 4 * (x + y * subWidth);
				const int id = 4 * (x + y * width);

				pixels[offset + 0] = pixels[id + 0];
				pixels[offset + 1] = pixels[id + 1];
				pixels[offset + 2] = pixels[id + 2];
				pixels[offset + 3] = pixels[id + 3];
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	void flipImageVertically(T* pixels, int width, int height, int channels = 4)
	{
		// Flip the image vertically
		for (int y = 0; y < height / 2; ++y)
		{
			const int swapY = height - y - 1;
			for (int x = 0; x < width; ++x)
			{
				const int offset = channels * (x + y * width);
				const int swapOffset = channels * (x + swapY * width);

				// Swap the 2 pixels
				for (int c = 0; c < channels; ++c)
					std::swap(pixels[offset + c], pixels[swapOffset + c]);
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	void linearizeDepthImage(unsigned char* pixels, int width, int height, float near, float far)
	{
		// Convert the float to unsigned bytes
		for (int y = 0; y < height; ++y)
		{
			for (int x = 0; x < width; ++x)
			{
				const int offset = 4 * (x + y * width);

				float depth = (*(float*)&pixels[offset]);
				float linearDepth = linearizeDepth(depth, near, far);

				pixels[offset + 0] = (unsigned char)(linearDepth * 255.0f);
				pixels[offset + 1] = (unsigned char)(linearDepth * 255.0f);
				pixels[offset + 2] = (unsigned char)(linearDepth * 255.0f);
				pixels[offset + 3] = 255;
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	size_t imageIndex(size_t x, size_t y, size_t c, size_t width, size_t height, size_t channels, bool flipUD)
	{
		return flipUD ?
			((height - y - 1) * width + x) * channels + c :
			(y * width + x) * channels + c;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool saveImage(Scene::Scene& scene, std::string filePath, size_t width, size_t height, size_t channels, unsigned char* pixels, bool flipUD)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, filePath);

		Debug::log_trace() << "Saving image: " << filePath << Debug::end;

		// Compute the full file name
		std::string fullFileName = (EnginePaths::assetsFolder() / filePath).string();

		// Create the parent folder hierarchy
		EnginePaths::makeDirectoryStructure(fullFileName, true);

		// Convert the pixels to uchar
		unsigned char* ucpixels = new unsigned char[4 * width * height];

		for (int y = 0; y < height; ++y)
		for (int x = 0; x < width; ++x)
		{
			for (size_t c = 0; c < channels; ++c)
			{
				size_t outIndex = imageIndex(x, y, c, width, height, 4, flipUD);
				size_t inIndex = imageIndex(x, y, c, width, height, channels, false);

				ucpixels[outIndex] = pixels[inIndex];
			}

			for (size_t c = channels; c < 4; ++c)
			{
				size_t outIndex = imageIndex(x, y, c, width, height, 4, flipUD);
				size_t inIndex = imageIndex(x, y, channels - 1, width, height, channels, false);

				ucpixels[outIndex] = (c == 3) ? 255 : pixels[inIndex];
			}
		}

		// Actually save the image
		auto result = stbi_write_png(fullFileName.c_str(), width, height, 4, ucpixels, 0);

		Debug::log_trace() << "Successfully saved image: " << filePath << Debug::end;

		return false;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool saveImage(Scene::Scene& scene, std::string filePath, size_t width, size_t height, size_t channels, float* pixels, bool flipUD)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, filePath);

		Debug::log_trace() << "Saving image: " << filePath << Debug::end;

		// Compute the full file name
		std::string fullFileName = (EnginePaths::assetsFolder() / filePath).string();

		// Create the parent folder hierarchy
		EnginePaths::makeDirectoryStructure(fullFileName, true);

		// Convert the pixels to uchar
		unsigned char* ucpixels = new unsigned char[4 * width * height];

		for (int y = 0; y < height; ++y)
		for (int x = 0; x < width; ++x)
		{
			for (size_t c = 0; c < channels; ++c)
			{
				size_t outIndex = imageIndex(x, y, c, width, height, 4, flipUD);
				size_t inIndex = imageIndex(x, y, c, width, height, channels, false);

				ucpixels[outIndex] = pixels[inIndex];
			}

			for (size_t c = channels; c < 4; ++c)
			{
				size_t outIndex = imageIndex(x, y, c, width, height, 4, flipUD);
				size_t inIndex = imageIndex(x, y, channels - 1, width, height, channels, false);

				ucpixels[outIndex] = (c == 3) ? 255 : pixels[inIndex];
			}
		}

		// Actually save the image
		auto result = stbi_write_png(fullFileName.c_str(), width, height, 4, ucpixels, 0);

		Debug::log_trace() << "Successfully saved image: " << filePath << Debug::end;

		return false;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool saveImage(Scene::Scene& scene, std::string filePath, cv::Mat const& image, bool flipUD)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, filePath);

		Debug::log_trace() << "Saving image: " << filePath << Debug::end;

		// Compute the full file name
		std::string fullFileName = (EnginePaths::assetsFolder() / filePath).string();

		// Create the parent folder hierarchy
		EnginePaths::makeDirectoryStructure(fullFileName, true);

		// Convert the pixels to uchar
		unsigned char* ucpixels = new unsigned char[4 * image.rows * image.cols];

		for (size_t y = 0; y < image.rows; ++y)
		for (size_t x = 0; x < image.cols; ++x)
		for (size_t c = 0; c < 4; ++c)
		{
			size_t outIndex = imageIndex(x, y, c, image.cols, image.rows, 4, flipUD);
			ucpixels[outIndex] = c == 3 ? 255 : (unsigned char)(image.at<float>(y, x) * 255);
		}

		// Actually save the image
		auto result = stbi_write_png(fullFileName.c_str(), image.cols, image.rows, 4, ucpixels, 0);

		Debug::log_trace() << "Successfully saved image: " << filePath << Debug::end;

		return false;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool saveImage(Scene::Scene& scene, std::string filePath, Eigen::MatrixXf const& image, bool flipUD)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, filePath);

		Debug::log_trace() << "Saving image: " << filePath << Debug::end;

		// Compute the full file name
		std::string fullFileName = (EnginePaths::assetsFolder() / filePath).string();

		// Create the parent folder hierarchy
		EnginePaths::makeDirectoryStructure(fullFileName, true);

		// Convert the pixels to uchar
		unsigned char* ucpixels = new unsigned char[4 * image.rows() * image.cols()];

		for (size_t y = 0; y < image.rows(); ++y)
		for (size_t x = 0; x < image.cols(); ++x)
		for (size_t c = 0; c < 4; ++c)
		{
			size_t outIndex = imageIndex(x, y, c, image.cols(), image.rows(), 4, flipUD);
			ucpixels[outIndex] = c == 3 ? 255 : (unsigned char)(image(y, x) * 255.0f);
		}

		// Actually save the image
		auto result = stbi_write_png(fullFileName.c_str(), image.cols(), image.rows(), 4, ucpixels, 0);

		Debug::log_trace() << "Successfully saved image: " << filePath << Debug::end;

		return false;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool saveImage(Scene::Scene& scene, std::string filePath, Eigen::MatrixXd const& image, bool flipUD)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, filePath);

		Debug::log_trace() << "Saving image: " << filePath << Debug::end;

		// Compute the full file name
		std::string fullFileName = (EnginePaths::assetsFolder() / filePath).string();

		// Create the parent folder hierarchy
		EnginePaths::makeDirectoryStructure(fullFileName, true);

		// Convert the pixels to uchar
		unsigned char* ucpixels = new unsigned char[4 * image.rows() * image.cols()];

		for (size_t y = 0; y < image.rows(); ++y)
		for (size_t x = 0; x < image.cols(); ++x)
		for (size_t c = 0; c < 4; ++c)
		{
			size_t outIndex = imageIndex(x, y, c, image.cols(), image.rows(), 4, flipUD);
			ucpixels[outIndex] = c == 3 ? 255 : (unsigned char)(image(y, x) * 255.0);
		}

		// Actually save the image
		auto result = stbi_write_png(fullFileName.c_str(), image.cols(), image.rows(), 4, ucpixels, 0);

		Debug::log_trace() << "Successfully saved image: " << filePath << Debug::end;

		return false;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool saveImage(Scene::Scene& scene, std::string filePath, Eigen::MatrixX<unsigned char> const& image, bool flipUD)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, filePath);

		Debug::log_trace() << "Saving image: " << filePath << Debug::end;

		// Compute the full file name
		std::string fullFileName = (EnginePaths::assetsFolder() / filePath).string();

		// Create the parent folder hierarchy
		EnginePaths::makeDirectoryStructure(fullFileName, true);

		// Convert the pixels to uchar
		unsigned char* ucpixels = new unsigned char[4 * image.rows() * image.cols()];

		for (size_t y = 0; y < image.rows(); ++y)
		for (size_t x = 0; x < image.cols(); ++x)
		for (size_t c = 0; c < 4; ++c)
		{
			size_t outIndex = imageIndex(x, y, c, image.cols(), image.rows(), 4, flipUD);
			ucpixels[outIndex] = c == 3 ? 255 : image(y, x);
		}

		// Actually save the image
		auto result = stbi_write_png(fullFileName.c_str(), image.cols(), image.rows(), 4, ucpixels, 0);

		Debug::log_trace() << "Successfully saved image: " << filePath << Debug::end;

		return false;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool saveTexture(Scene::Scene& scene, const std::string& textureName, std::string filePath)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, filePath);

		// Make sure it actually exists.
		if (scene.m_textures.find(textureName) == scene.m_textures.end())
			return false;

		Debug::log_trace() << "Saving texture: " << textureName << " to " << filePath << Debug::end;

		// Compute the full file name
		std::string fullFileName = (EnginePaths::assetsFolder() / filePath).string();

		// Create the parent folder hierarchy
		EnginePaths::makeDirectoryStructure(fullFileName, true);

		// Make sure the texture has been updated
		glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);

		// Get the image dimensions
		int w = scene.m_textures[textureName].m_width;
		int h = scene.m_textures[textureName].m_height;

		// Allocate memory
		unsigned char* pixels = new unsigned char[4 * w * h];

		// Extract the image
		glBindTexture(GL_TEXTURE_2D, scene.m_textures[textureName].m_texture);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
		glBindTexture(GL_TEXTURE_2D, 0);

		// Flip the image vertically
		flipImageVertically(pixels, w, h);

		// Now actually save the texture
		auto result = stbi_write_png(fullFileName.c_str(), w, h, 4, pixels, 0);

		// Free the pixels
		delete pixels;

		Debug::log_trace() << "Successfully saved texture: " << textureName << " to " << filePath << Debug::end;

		return result == 0;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool saveDefaultFramebuffer(Scene::Scene& scene, std::string filePath, int width, int height)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, filePath);

		filePath = filePath + ".png";

		Debug::log_trace() << "Saving default framebuffer to " << filePath << Debug::end;

		// Compute the full file name
		std::string fullFileName = (EnginePaths::assetsFolder() / filePath).string();

		// Create the parent folder hierarchy
		EnginePaths::makeDirectoryStructure(fullFileName, true);

		// Make sure the texture has been updated
		glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);

		// Allocate memory
		std::unique_ptr<unsigned char[]> pixels(new unsigned char[4 * width * height]);

		// Extract the image
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.get());

		// Flip the image vertically
		flipImageVertically(pixels.get(), width, height);

		// Now actually save the texture
		auto result = stbi_write_png(fullFileName.c_str(), width, height, 4, pixels.get(), 0);

		Debug::log_trace() << "Successfully saved default framebuffer to " << filePath << Debug::end;

		return result == 0;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool saveGbufferColor(Scene::Scene& scene, std::string filePath, int gbufferId, int layerId, int width, int height)
	{
		filePath = filePath + "-color.png";

		Profiler::ScopedCpuPerfCounter perfCounter(scene, filePath);

		Debug::log_trace() << "Saving gbuffer color channel to " << filePath << Debug::end;

		// Compute the full file name
		std::string fullFileName = (EnginePaths::assetsFolder() / filePath).string();

		// Create the parent folder hierarchy
		EnginePaths::makeDirectoryStructure(fullFileName, true);

		// Make sure the texture has been updated
		glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);

		// Allocate memory
		size_t bufSize = 4 * scene.m_gbuffer[gbufferId].m_width * scene.m_gbuffer[gbufferId].m_height;
		std::unique_ptr<unsigned char[]> pixels(new unsigned char[bufSize]);

		// Extract the color buffer
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glGetTextureSubImage(scene.m_gbuffer[gbufferId].m_colorTextures[1 - scene.m_gbuffer[gbufferId].m_writeBuffer], 0, 0, 0, layerId, scene.m_gbuffer[gbufferId].m_width, scene.m_gbuffer[gbufferId].m_height, 1, GL_RGBA, GL_UNSIGNED_BYTE, bufSize, pixels.get());

		// Extract the relevant subimage
		extractSubImage(pixels.get(), scene.m_gbuffer[gbufferId].m_width, scene.m_gbuffer[gbufferId].m_height, width, height);

		// Flip the image vertically
		flipImageVertically(pixels.get(), width, height);

		// Now actually save the texture
		auto result = stbi_write_png(fullFileName.c_str(), width, height, 4, pixels.get(), 0);

		Debug::log_trace() << "Successfully saved gbuffer color channel to " << filePath << Debug::end;

		return result == 0;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool saveGbufferNormal(Scene::Scene& scene, std::string filePath, int gbufferId, int layerId, int width, int height)
	{
		filePath = filePath + "-normal.png";

		Profiler::ScopedCpuPerfCounter perfCounter(scene, filePath);

		Debug::log_trace() << "Saving gbuffer normal channel (in display format) to " << filePath << Debug::end;

		// Compute the full file name
		std::string fullFileName = (EnginePaths::assetsFolder() / filePath).string();

		// Create the parent folder hierarchy
		EnginePaths::makeDirectoryStructure(fullFileName, true);

		// Make sure the texture has been updated
		glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);

		// Allocate memory
		size_t bufSize = 4 * scene.m_gbuffer[gbufferId].m_width * scene.m_gbuffer[gbufferId].m_height;
		std::unique_ptr<unsigned char[]> pixels(new unsigned char[bufSize]);

		// Extract the normal buffer
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glGetTextureSubImage(scene.m_gbuffer[gbufferId].m_normalTexture, 0, 0, 0, layerId, scene.m_gbuffer[gbufferId].m_width, scene.m_gbuffer[gbufferId].m_height, 1, GL_RGBA, GL_UNSIGNED_BYTE, bufSize, pixels.get());

		// Extract the relevant subimage
		extractSubImage(pixels.get(), scene.m_gbuffer[gbufferId].m_width, scene.m_gbuffer[gbufferId].m_height, width, height);

		// Flip the image vertically
		flipImageVertically(pixels.get(), width, height);

		// Now actually save the texture
		auto result = stbi_write_png(fullFileName.c_str(), width, height, 4, pixels.get(), 0);

		Debug::log_trace() << "Successfully saved gbuffer normal channel to " << filePath << Debug::end;

		return result == 0;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool saveGbufferLinearDepth(Scene::Scene& scene, std::string filePath, int gbufferId, int layerId, int width, int height, float near, float far)
	{
		filePath = filePath + "-linear-depth.png";

		Profiler::ScopedCpuPerfCounter perfCounter(scene, filePath);

		Debug::log_trace() << "Saving gbuffer depth channel to " << filePath << Debug::end;

		// Compute the full file name
		std::string fullFileName = (EnginePaths::assetsFolder() / filePath).string();

		// Create the parent folder hierarchy
		EnginePaths::makeDirectoryStructure(fullFileName, true);

		// Make sure the texture has been updated
		glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);

		// Allocate memory
		size_t bufSize = 4 * scene.m_gbuffer[gbufferId].m_width * scene.m_gbuffer[gbufferId].m_height;
		std::unique_ptr<unsigned char[]> pixels(new unsigned char[bufSize]);

		// Extract the depth buffer
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glGetTextureSubImage(scene.m_gbuffer[gbufferId].m_depthTexture, 0, 0, 0, layerId, scene.m_gbuffer[gbufferId].m_width, scene.m_gbuffer[gbufferId].m_height, 1, GL_DEPTH_COMPONENT, GL_FLOAT, bufSize, pixels.get());

		// Extract the relevant subimage
		extractSubImage(pixels.get(), scene.m_gbuffer[gbufferId].m_width, scene.m_gbuffer[gbufferId].m_height, width, height);

		// Flip the image vertically
		flipImageVertically(pixels.get(), width, height);

		// Linearize the depth image
		linearizeDepthImage(pixels.get(), width, height, near, far);

		// Now actually save the texture
		auto result = stbi_write_png(fullFileName.c_str(), width, height, 4, pixels.get(), 0);

		Debug::log_trace() << "Successfully saved gbuffer depth channel to " << filePath << Debug::end;

		return result == 0;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool saveGbuffer(Scene::Scene& scene, const std::string& filePath, int gbufferId, int numLayers, int width, int height, float near, float far)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, filePath);

		Debug::log_trace() << "Saving gbuffer to " << filePath << Debug::end;

		bool result = true;
		for (size_t i = 0; i < numLayers; ++i)
		{
			result &= saveGbufferColor(scene, filePath, gbufferId, i, width, height);
			result &= saveGbufferNormal(scene, filePath, gbufferId, i, width, height);
			result &= saveGbufferLinearDepth(scene, filePath, gbufferId, i, width, height, near, far);
		}

		if (result)
		{
			Debug::log_trace() << "Successfully saved gbuffer to " << filePath << Debug::end;
		}
		else
		{
			Debug::log_error() << "Error saving gbuffer to " << filePath << Debug::end;
		}

		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string generateMeshTexturePath(std::string const& meshName, std::string const& textureName)
	{
		const auto strBegin = textureName.find_first_not_of(" \t");
		const auto strEnd = textureName.find_last_not_of(" \t");
		const auto strRange = strEnd - strBegin + 1;

		std::string trimmedPath = textureName.substr(strBegin, strRange);
		return "Textures/Mesh/" + meshName + "/" + trimmedPath;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool loadMaterialTexture(Scene::Scene& scene, std::string const& baseName, aiMaterial* pMaterial, std::vector<aiTextureType> const& textureTypes, std::string& materialTexturePath, std::string defaultPath)
	{
		// Go through each possible texture type
		for (auto textureType : textureTypes)
		{
			// Check if the specified texture is present
			if (pMaterial->GetTextureCount(textureType) > 0)
			{
				// Extract the path
				aiString path;
				pMaterial->GetTexture(textureType, 0, &path);

				// Generate the full file path
				materialTexturePath = generateMeshTexturePath(baseName, path.data);

				// Try to load the corresponding texture
				if (loadTexture(scene, materialTexturePath, materialTexturePath))
					return true;
			}
		}

		// Fall back to the default texture
		materialTexturePath = defaultPath;

		// Not present
		return true;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool loadMesh(Scene::Scene& scene, const std::string& filePath)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, filePath);

		// Make sure it isn't loaded already.
		if (scene.m_meshes.find(filePath) != scene.m_meshes.end())
			return true;

		Debug::log_trace() << "Loading mesh: " << filePath << Debug::end;

		// Compute the full file name
		std::filesystem::path fullFilePath = EnginePaths::assetsFolder() / "Meshes" / filePath;
		std::string const& fullFileName = fullFilePath.string();

		// Extract the mesh base name
		std::string const& extension = fullFilePath.extension().string();
		std::string const& meshBaseName = fullFilePath.stem().string();

		// Whether the object uses PBR materials or not
		// TODO: extend with more extensions
		bool isPbr = extension != ".obj";

		// Try to load the mesh.
		Assimp::Importer importer;

		const aiScene* pScene = importer.ReadFile(fullFileName.c_str(),
			aiProcess_Triangulate | aiProcess_FixInfacingNormals |
			aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace | 
			aiProcess_JoinIdenticalVertices);

		// Make sure it was successful.
		if (pScene == nullptr)
		{
			Debug::log_error() << "Error trying to load mesh: " << filePath << Debug::end;
			return false;
		}

		// base name for the mesh
		std::string baseName = filePath.substr(0, filePath.find_last_of('.'));

		// The created mesh object
		GPU::Mesh mesh;

		// Init the AABB vertices
		mesh.m_aabb = BVH::AABB(glm::vec3(FLT_MAX), glm::vec3(-FLT_MAX));

		// Extract the materials of the mesh
		auto& materials = mesh.m_materials;
		materials.resize(pScene->mNumMaterials);

		for (size_t materialId = 0; materialId < pScene->mNumMaterials; ++materialId)
		{
			// Extract the material object.
			aiMaterial* pMaterial = pScene->mMaterials[materialId];
			auto& material = materials[materialId];

			// Extract the name of the material
			aiString matName;
			pMaterial->Get(AI_MATKEY_NAME, matName);

			// Store the name of the material
			material.m_name = meshBaseName + "_" + (matName.length > 0 ? matName.data : "material" + std::to_string(materialId));

			// Load the textures
			loadMaterialTexture(scene, baseName, pMaterial, { aiTextureType_DIFFUSE }, material.m_diffuseMap, "default_diffuse_map");
			loadMaterialTexture(scene, baseName, pMaterial, { aiTextureType_NORMALS, aiTextureType_HEIGHT }, material.m_normalMap, "default_normal_map");
			loadMaterialTexture(scene, baseName, pMaterial, { aiTextureType_SPECULAR, aiTextureType_UNKNOWN }, material.m_specularMap, "default_specular_map");
			loadMaterialTexture(scene, baseName, pMaterial, { aiTextureType_OPACITY }, material.m_alphaMap, "default_alpha_map");
			loadMaterialTexture(scene, baseName, pMaterial, { aiTextureType_DISPLACEMENT }, material.m_displacementMap, "default_displacement_map");

			// extract the roughness
			aiColor3D v;
			float f;
			if (pMaterial->Get(AI_MATKEY_SHININESS, f) == AI_SUCCESS)
			{
				material.m_roughness = 1.0f - (f / 2048.0f);
			}
			else
			{
				material.m_roughness = 0.0f;
			}

			if (pMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, v) == AI_SUCCESS)
			{
				material.m_diffuse = glm::vec3(v.r, v.g, v.b);
			}
			else
			{
				material.m_diffuse = glm::vec3(1.0f);
			}

			if (pMaterial->Get(AI_MATKEY_COLOR_SPECULAR, v) == AI_SUCCESS)
			{
				material.m_specular = (v.r + v.g + v.b) / 3.0f;
			}
			else
			{
				material.m_specular = 1.0f;
			}

			if (pMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, v) == AI_SUCCESS)
			{
				material.m_emissive = glm::vec3(v.r, v.g, v.b);
			}
			else
			{
				material.m_emissive = glm::vec3(1.0f);
			}

			if (pMaterial->Get(AI_MATKEY_OPACITY, material.m_opacity) != AI_SUCCESS)
			{
				material.m_opacity = 1.0f;
			}

			if (pMaterial->Get(AI_MATKEY_TWOSIDED, material.m_twoSided) != AI_SUCCESS)
			{
				material.m_twoSided = false;
			}

			// Set the specular mask for PBR material
			if (isPbr) material.m_specularMask = glm::vec4(0.0f);

			// Set the material blend mode
			if (material.m_alphaMap != "default_alpha_map" || material.m_opacity < 1.0f) 
				material.m_blendMode = GPU::Material::Translucent;

			// Store the material in the scane
			scene.m_materials[material.m_name] = material;
		}

		// Compute the total number of vertices and indices
		int numTotalVertices = 0;
		int numTotalIndices = 0;

		for (size_t subMeshId = 0; subMeshId < pScene->mNumMeshes; ++subMeshId)
		{
			numTotalVertices += pScene->mMeshes[subMeshId]->mNumVertices;
			numTotalIndices += pScene->mMeshes[subMeshId]->mNumFaces * 3;
		}

		// Monolithic buffers
		std::vector<glm::vec3> allPositions(numTotalVertices);
		std::vector<glm::vec3> allNormals(numTotalVertices);
		std::vector<glm::vec3> allTangents(numTotalVertices);
		std::vector<glm::vec3> allBitangents(numTotalVertices);
		std::vector<glm::vec2> allUvs(numTotalVertices);
		std::vector<unsigned> allIndices(numTotalIndices);
		std::vector<unsigned> allMaterialIndices(numTotalIndices / 3);
		int monolithicVertexId = 0;
		int monolithicIndexId = 0;

		// Extract the mesh data.
		auto& subMeshes = mesh.m_subMeshes;
		subMeshes.resize(pScene->mNumMeshes);

		for (size_t subMeshId = 0; subMeshId < pScene->mNumMeshes; ++subMeshId)
		{
			// Extract the sub mesh object.
			aiMesh* pSubMesh = pScene->mMeshes[subMeshId];
			auto& subMesh = subMeshes[subMeshId];

			// Extract the relevant info.
			subMesh.m_name = pSubMesh->mName.C_Str();
			subMesh.m_materialId = pSubMesh->mMaterialIndex;
			subMesh.m_vertexCount = pSubMesh->mNumVertices;
			subMesh.m_indexCount = pSubMesh->mNumFaces * 3;
			subMesh.m_vertexStartID = monolithicVertexId;
			subMesh.m_indexStartID = monolithicIndexId;

			// Init the AABB vertices
			subMesh.m_aabb = BVH::AABB(glm::vec3(FLT_MAX), glm::vec3(-FLT_MAX));

			// Extract the vertices.
			int vertexStartId = monolithicVertexId;
			int indexStartId = monolithicIndexId;

			// Extract the vertices
			for (size_t vertexId = 0; vertexId < pSubMesh->mNumVertices; ++vertexId)
			{
				allPositions[vertexStartId + vertexId] = glm::vec3(pSubMesh->mVertices[vertexId].x, pSubMesh->mVertices[vertexId].y, pSubMesh->mVertices[vertexId].z);
				allNormals[vertexStartId + vertexId] = glm::vec3(pSubMesh->mNormals[vertexId].x, pSubMesh->mNormals[vertexId].y, pSubMesh->mNormals[vertexId].z);
				allTangents[vertexStartId + vertexId] = glm::vec3(pSubMesh->mTangents[vertexId].x, pSubMesh->mTangents[vertexId].y, pSubMesh->mTangents[vertexId].z);
				allBitangents[vertexStartId + vertexId] = glm::vec3(pSubMesh->mBitangents[vertexId].x, pSubMesh->mBitangents[vertexId].y, pSubMesh->mBitangents[vertexId].z);
				if (pSubMesh->HasTextureCoords(0)) allUvs[vertexStartId + vertexId] = glm::vec2(pSubMesh->mTextureCoords[0][vertexId].x, pSubMesh->mTextureCoords[0][vertexId].y);

				// Update the AABB
				subMesh.m_aabb = subMesh.m_aabb.extend(allPositions[vertexStartId + vertexId]);
			}

			// Extract the indicies.
			for (size_t faceId = 0; faceId < pSubMesh->mNumFaces; ++faceId)
			{
				aiFace* face = &pSubMesh->mFaces[faceId];

				allIndices[indexStartId + faceId * 3] = face->mIndices[0];
				allIndices[indexStartId + faceId * 3 + 1] = face->mIndices[1];
				allIndices[indexStartId + faceId * 3 + 2] = face->mIndices[2];

				allMaterialIndices[indexStartId / 3 + faceId] = subMesh.m_materialId;
			}

			monolithicVertexId += pSubMesh->mNumVertices;
			monolithicIndexId += pSubMesh->mNumFaces * 3;

			mesh.m_aabb = mesh.m_aabb.extend(subMesh.m_aabb);
		}

		// Store the final vertex and index counts
		mesh.m_indexCount = monolithicIndexId;
		mesh.m_vertexCount = monolithicVertexId;

		// Generate and fill the moonlithic buffers
		glGenBuffers(1, &mesh.m_vboPosition);
		glBindBuffer(GL_ARRAY_BUFFER, mesh.m_vboPosition);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * allPositions.size(), allPositions.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glGenBuffers(1, &mesh.m_vboNormal);
		glBindBuffer(GL_ARRAY_BUFFER, mesh.m_vboNormal);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * allNormals.size(), allNormals.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glGenBuffers(1, &mesh.m_vboTangent);
		glBindBuffer(GL_ARRAY_BUFFER, mesh.m_vboTangent);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * allTangents.size(), allTangents.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glGenBuffers(1, &mesh.m_vboBitangent);
		glBindBuffer(GL_ARRAY_BUFFER, mesh.m_vboBitangent);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * allBitangents.size(), allBitangents.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glGenBuffers(1, &mesh.m_vboUV);
		glBindBuffer(GL_ARRAY_BUFFER, mesh.m_vboUV);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * allUvs.size(), allUvs.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glGenBuffers(1, &mesh.m_mbo);
		glBindBuffer(GL_ARRAY_BUFFER, mesh.m_mbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(unsigned) * allMaterialIndices.size(), allMaterialIndices.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glGenBuffers(1, &mesh.m_ibo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.m_ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned) * allIndices.size(), allIndices.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		// Configure the VAO
		glCreateVertexArrays(1, &mesh.m_vao);
		glBindVertexArray(mesh.m_vao);
		glBindBuffer(GL_ARRAY_BUFFER, mesh.m_vboPosition);
		glEnableVertexAttribArray(GPU::VertexAttribIndices::VERTEX_ATTRIB_POSITION);
		glVertexAttribPointer(GPU::VertexAttribIndices::VERTEX_ATTRIB_POSITION, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, mesh.m_vboNormal);
		glEnableVertexAttribArray(GPU::VertexAttribIndices::VERTEX_ATTRIB_NORMAL);
		glVertexAttribPointer(GPU::VertexAttribIndices::VERTEX_ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, mesh.m_vboTangent);
		glEnableVertexAttribArray(GPU::VertexAttribIndices::VERTEX_ATTRIB_TANGENT);
		glVertexAttribPointer(GPU::VertexAttribIndices::VERTEX_ATTRIB_TANGENT, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, mesh.m_vboBitangent);
		glEnableVertexAttribArray(GPU::VertexAttribIndices::VERTEX_ATTRIB_BITANGENT);
		glVertexAttribPointer(GPU::VertexAttribIndices::VERTEX_ATTRIB_BITANGENT, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, mesh.m_vboUV);
		glEnableVertexAttribArray(GPU::VertexAttribIndices::VERTEX_ATTRIB_UV);
		glVertexAttribPointer(GPU::VertexAttribIndices::VERTEX_ATTRIB_UV, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.m_ibo);
		glBindVertexArray(0);

		// Associate the proper label to them
		std::string label = baseName + "_vao";
		glObjectLabel(GL_VERTEX_ARRAY, mesh.m_vao, label.length(), label.c_str());

		// Store the mesh.
		scene.m_meshes[filePath] = mesh;

		Debug::log_trace() << "Successfully loaded mesh: " << filePath << Debug::end;

		return true;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool loadTfModel(Scene::Scene& scene, const std::string& modelName, TensorFlow::ModelSpec const& modelSpec)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, modelName);

		// Make sure it isn't loaded already.
		if (scene.m_tfModels.find(modelName) != scene.m_tfModels.end())
			return true;

		// Construct the necessary model paths
		std::filesystem::path modelPath = (EnginePaths::assetsFolder() / "Generated" / "Networks" / modelName).string();
		std::string modelFolder = modelPath.string();

		Debug::log_trace() << "Loading TensorFlow model: " << modelName << " (from: " << modelFolder << ")" << Debug::end;

		// Try to load the model
		auto model = TensorFlow::restoreSavedModel(modelName, modelFolder, modelSpec);

		// Make sure it was successfully loaded
		if (!model.has_value())
		{
			Debug::log_error() << "Error trying to load TensorFlow model: " << modelName << Debug::end;
			return false;
		}

		// Store the restored model
		scene.m_tfModels[modelName] = model.value();

		// Mark the success
		return true;
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string generateShaderDefines(const std::vector<std::string>& defines)
	{
		std::stringstream ss;

		for (size_t i = 0; i < defines.size(); ++i)
			ss << "#define " << defines[i] << std::endl;

		return ss.str();
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string shaderTypeToString(GLenum shaderType)
	{
		switch (shaderType)
		{
		case GL_TESS_CONTROL_SHADER: return "TESS_CONTROL_SHADER";
		case GL_TESS_EVALUATION_SHADER: return "TESS_EVALUATION_SHADER";
		case GL_VERTEX_SHADER: return "VERTEX_SHADER";
		case GL_GEOMETRY_SHADER: return "GEOMETRY_SHADER";
		case GL_FRAGMENT_SHADER: return "FRAGMENT_SHADER";
		case GL_COMPUTE_SHADER: return "COMPUTE_SHADER";
		};

		return "#ERROR";
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string mainProgramName(std::string const& fileName)
	{
		std::string const& base = std::filesystem::path(fileName).stem().string(); // Extract the base program name
		return base.substr(0, base.find_last_of('_')); // Strip the shader the type identifier
	}

	////////////////////////////////////////////////////////////////////////////////
	std::vector<std::string> generateBuiltinDefines(Scene::Scene& scene, GLenum shaderType, const std::string& shaderName, const std::string& fileName)
	{
		return std::vector<std::string>
		{
			"SHADER_TYPE_"s + shaderTypeToString(shaderType),
			"SHADER_TYPE "s + shaderTypeToString(shaderType),
			"MAIN_PROGRAM " + mainProgramName(fileName)
		};
	}

	////////////////////////////////////////////////////////////////////////////////
	using IncludeCondTerm = std::array<std::string, 3>;
	using IncludeCondExpression = std::tuple<IncludeCondTerm, std::string, IncludeCondTerm>;

	////////////////////////////////////////////////////////////////////////////////
	std::string lookupShaderDefine(std::string operand, ShaderParameters const& shaderParameters, std::unordered_map<std::string, std::string> const& defines)
	{
		for (auto it = defines.find(operand); it != defines.end(); it = defines.find(operand))
		{
			//Debug::log_info() << " - substituting " << operand << " with (" << it->first << ", " << it->second << ")" << Debug::end;
			operand = it->second;
		}
		return operand;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool evalShaderIncludeCondTerm(IncludeCondTerm const& term, ShaderParameters const& shaderParameters, std::unordered_map<std::string, std::string> const& defines)
	{
		auto [op1, op, op2] = term;
		//Debug::log_info() << "Evaluating term: " << term << Debug::end;
		op1 = lookupShaderDefine(op1, shaderParameters, defines);
		op2 = lookupShaderDefine(op2, shaderParameters, defines);
		//Debug::log_info() << " - final operands: " << op1 << ", " << op << ", " << op2 << Debug::end;
		if (op == "==") return op1 == op2;
		if (op == "!=") return op1 != op2;
		Debug::log_error() << "Unable to interpret operator \"" << op << "\" in expression: " << term << Debug::end;
		return false;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool evalShaderIncludeCondExpression(IncludeCondExpression const& expression, ShaderParameters const& shaderParameters, std::unordered_map<std::string, std::string> const& defines)
	{
		auto const& [term1, op, term2] = expression;
		const bool tb1 = evalShaderIncludeCondTerm(term1, shaderParameters, defines);
		const bool tb2 = evalShaderIncludeCondTerm(term2, shaderParameters, defines);
		if (op == "&&") return tb1 && tb2;
		if (op == "&&") return tb1 || tb2;
		Debug::log_error() << "Unable to interpret operator \"" << op << "\" in expression: { " << term1 << ", " << op << ", " << term2 << " }" << Debug::end;
		return false;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool evalShaderIncludeCond(std::string cond, ShaderParameters const& shaderParameters, std::unordered_map<std::string, std::string> const& defines)
	{
		static const std::string s_space = "[[:space:]]*";
		static const std::string s_termOperand = "([_[:alnum:]]+)";
		static const std::string s_termOperator = "([\\=\\!\\<\\>]+)";
		static const std::string s_term = s_termOperand + s_space + s_termOperator + s_space + s_termOperand;
		static const std::string s_expOperand = s_term;
		static const std::string s_expOperator = "([\\&\\|]+)";
		static const std::string s_expression = s_expOperand + s_space + s_expOperator + s_space + s_expOperand;

		static const std::regex s_termRegex = std::regex(s_term);
		static const std::regex s_expressionRegex = std::regex(s_expression);

		bool result = true;

		std::smatch capture;
		while (std::regex_search(cond, capture, s_expressionRegex))
		{
			// capture[1:3]: term #1, capture[4]: operator, capture[5:7]: term #2
			const IncludeCondTerm term1 = { capture[1], capture[2], capture[3] };
			const IncludeCondTerm term2 = { capture[5], capture[6], capture[7] };
			const IncludeCondExpression exp = { term1, capture[4], term2 };
			result &= evalShaderIncludeCondExpression(exp, shaderParameters, defines);
			cond = capture.suffix();
		}
		
		// Process the remaining term
		if (std::regex_search(cond, capture, s_termRegex))
		{
			// capture[1:3]: term
			const IncludeCondTerm term = { capture[1], capture[2], capture[3] };
			result &= evalShaderIncludeCondTerm(term, shaderParameters, defines);
			cond = capture.suffix();
		}

		// Make sure the term was fully validated
		if (!cond.empty())
		{
			Debug::log_error() << "Invalid #includeif condition; unprocessed term: " << cond << Debug::end;
			return false;
		}

		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string generateFullShaderSource(Scene::Scene& scene, GLenum shaderType, 
		const std::string& shaderName, const std::string& fileName, ShaderParameters const& shaderParameters)
	{
		// Output string stream
		std::string version;
		std::stringstream requiredExtensionsStream;
		std::stringstream optionalExtensionsStream;
		std::stringstream shaderSourceStream;

		// Lines that need processing
		std::vector<std::string> lines;

		// Sources that are already processed
		std::unordered_map<std::string, bool> processed;

		// List of defines
		std::unordered_map<std::string, std::string> defines;

		// Helper lambda for append a source file
		auto appendSource = [&](std::string const& source, std::string const& contents, std::vector<std::string>::iterator at)
		{
			auto inserter = std::inserter(lines, at);
			inserter++ = ("//! Start of contents for source \"" + source + "\"");
			inserter = std::copy(std::line_iterator<char>(contents), std::line_iterator<char>(), inserter);
			inserter++ = ("//! End of contents for source \"" + source + "\"");
		};

		// Helper lambda for loading a source file and splitting it up
		auto appendFile = [&](std::string const& fileName, std::vector<std::string>::iterator at)
		{
			// No double includes
			if (processed.find(fileName) != processed.end() || processed[fileName])
				return;

			// Store the processed flag
			processed[fileName] = true;

			// Construct the file names 
			std::filesystem::path const& fullFilePath = EnginePaths::assetsFolder() / fileName;
			std::string const& fileNameRelative = std::filesystem::relative(fullFilePath, EnginePaths::openGlShadersFolder()).string();

			// Try to load the file contents
			std::optional<std::string> source = loadTextFile(scene, fileName, true);
			if (source.has_value() == false) return;

			// Append the source
			appendSource(fileNameRelative, source.value(), at);
		};

		// Append the common defines
		appendSource("Builtin Defines", generateShaderDefines(generateBuiltinDefines(scene, shaderType, shaderName, fileName)), lines.end());
		appendSource("User Defines", generateShaderDefines(shaderParameters.m_defines), lines.end());
		appendSource("User Enums", generateShaderDefines(shaderParameters.m_enums), lines.end());
		appendSource("User Structs", generateShaderDefines(shaderParameters.m_structs), lines.end());

		// Append the main file
		appendFile(fileName, lines.end());

		// Process each line individually
		std::smatch capture;
		for (size_t i = 0; i < lines.size(); ++i)
		{
			// Various patterns
			static const std::regex s_versionRegex = std::regex("[[:space:]]*#version[[:space:]]+([[:digit:]]+)[[:space:]]*");
			static const std::regex s_extensionRegex = std::regex("[[:space:]]*#extension[[:space:]]+([_[:alnum:]]+)[[:space:]]*:[[:space:]]*([[:alnum:]]+)[[:space:]]*");
			static const std::regex s_defineRegex = std::regex("[[:space:]]*#define[[:space:]]+([_[:alnum:]]+)[[:space:]]+([_[:alnum:]]+)");
			static const std::regex s_includeRegex = std::regex("[[:space:]]*#include[[:space:]]+<(.*)>[[:space:]]*");
			static const std::regex s_includeIfRegex = std::regex("[[:space:]]*#includeif\\(([\\(\\)_&!|=[:alnum:][:space:]]*)\\)[[:space:]]+<(.*)>[[:space:]]*");

			// Extract the current line
			std::string const& line = lines[i];

			// Is this the version specifier?
			if (std::regex_match(line, capture, s_versionRegex))
			{
				version = line + "\n";
				shaderSourceStream << "//";
			}

			// Is this an extension specifier?
			else if (std::regex_match(line, capture, s_extensionRegex))
			{
				std::string const& name = capture[1];
				std::string const& state = capture[2];
				if (state == "require")
				{
					requiredExtensionsStream << line << "\n";
					requiredExtensionsStream << "#define EXT_" << name << "_ENABLED 1\n";
				}
				else if (state == "enable" || state == "warn")
				{
					optionalExtensionsStream << line << "\n";
					optionalExtensionsStream << "#define EXT_" << name << "_ENABLED 1\n";
				}
				shaderSourceStream << "//";
			}

			// Is this an include directive?
			else if (std::regex_match(line, capture, s_includeRegex))
			{
				std::string const& includeName = capture[1];
				if (processed.find(includeName) == processed.end() || processed[includeName] == false) // No double includes
					appendFile(includeName, lines.begin() + (i + 1));
				shaderSourceStream << "//";
			}

			// Is this a conditional include directive?
			else if (std::regex_match(line, capture, s_includeIfRegex))
			{
				std::string const& includeCond = capture[1];
				std::string const& includeName = capture[2];
				if (evalShaderIncludeCond(includeCond, shaderParameters, defines))
					if (processed.find(includeName) == processed.end() || processed[includeName] == false) // No double includes
						appendFile(includeName, lines.begin() + (i + 1));
				shaderSourceStream << "//";
			}

			// Is this a define directive?
			else if (std::regex_match(line, capture, s_defineRegex))
			{
				// Value of the define
				std::string const& defineName = capture[1];
				std::string const& defineVal = capture[2];
				defines[defineName] = defineVal;
			}

			// Pass through the line
			shaderSourceStream << lines[i] << std::endl;
		}

		// Assembly the full source
		std::stringstream fullShaderSourceStream;
		fullShaderSourceStream << version << std::endl;
		fullShaderSourceStream << "//! Required extensions" << std::endl;
		fullShaderSourceStream << requiredExtensionsStream.str() << std::endl;
		if (GPU::enableOptionalExtensions())
		{
			fullShaderSourceStream << "//! Optional extensions" << std::endl;
			fullShaderSourceStream << optionalExtensionsStream.str() << std::endl;
		}
		fullShaderSourceStream << "//! Shader source" << std::endl;
		fullShaderSourceStream << shaderSourceStream.str() << std::endl;
		return fullShaderSourceStream.str();
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string getShaderSource(Scene::Scene& scene, GLenum shaderType, const std::string& shaderName, const std::string& fileName, 
		ShaderParameters const& shaderParameters)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, fileName);

		Debug::log_trace() << "Generating shader source: " << fileName << Debug::end;

		// Try to load the shader file
		std::optional<std::string> source = loadTextFile(scene, fileName, false);
		if (!source.has_value())
		{
			Debug::log_error() << "Unable to load text file '" << fileName << "' referenced by '" << shaderName << "'" << Debug::end;
			return "";
		}

		return generateFullShaderSource(scene, shaderType, shaderName, fileName, shaderParameters );
	}

	////////////////////////////////////////////////////////////////////////////////
	std::pair<std::string, int> shaderFileErrorLogLineReference(std::string const& source, int lineRef)
	{
		// Which source we are in right now
		std::vector<std::pair<std::string, int>> prevSourceFiles;

		// Start and end file prefixes
		static const std::string startSourcePrefix = "//! Start of contents for source \"";
		static const std::string endSourcePrefix = "//! End of contents for source \"";

		// Current source name and line
		std::string sourceFileName = "";
		int sourceFileLine = 0;

		std::stringstream ssl(source);
		std::string line;
		for (int i = 1; std::getline(ssl, line); ++i)
		{
			// Start of a content file
			if (line.compare(0, startSourcePrefix.size(), startSourcePrefix) == 0)
			{
				prevSourceFiles.push_back(std::make_pair(sourceFileName, sourceFileLine));
				sourceFileName = line.substr(startSourcePrefix.size(), line.find_last_of("\"") - startSourcePrefix.size());

				sourceFileLine = 0;
			}

			// End of a content file
			else if (line.compare(0, endSourcePrefix.size(), endSourcePrefix) == 0)
			{
				sourceFileName = prevSourceFiles.back().first;
				sourceFileLine = prevSourceFiles.back().second;
				prevSourceFiles.pop_back();
			}
			else
			{
				// Increment the source file line id
				++sourceFileLine;
			}

			// Stop when we reach the desired line
			if (i == lineRef) break;
		}

		return std::make_pair(sourceFileName, sourceFileLine);
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string detailedShaderFileErrorLog(std::string const& source, std::string const& log, int printAreaSize = 2, bool printPeakWindows = false)
	{
		// Reference to a line in the shader
		static const std::regex s_lineReference = std::regex("0\\(([[:digit:]]+)\\)");

		// Create a string stream from the log message to process each line
		std::stringstream ss(log);

		// Create a string stream for the result
		std::stringstream result;

		// Process each line.
		std::smatch capture;
		for (std::string logLine; std::getline(ss, logLine);)
		{
			// The formatted log line to print
			std::string formattedLogLine;

			// Search start for regex match
			std::string::const_iterator searchStart = logLine.begin();

			// Source location for the error line
			int errorLineNo = -1;
			int errorPrefixSize = -1;
			int numMatches = 0;

			// Look for line references
			while (std::regex_search(searchStart, logLine.cend(), capture, s_lineReference))
			{
				// Appen the prefix to the formatted output
				formattedLogLine = formattedLogLine + std::string(searchStart, capture.prefix().second);

				// Increment the search start iterator
				searchStart = capture.suffix().first;

				// Extract the line number
				std::string lineNoStr = capture[1];
				int lineNo = std::atoi(lineNoStr.c_str());

				// Extract the file 
				auto[sourceFileName, sourceFileLine] = shaderFileErrorLogLineReference(source, lineNo);

				// Construct the location reference
				std::string locationReference = sourceFileName + " (" + std::to_string(sourceFileLine) + ")";

				// Append the location reference
				formattedLogLine = formattedLogLine + locationReference;

				// Store the error line number
				if (numMatches == 0)
				{
					errorLineNo = lineNo;
					errorPrefixSize = locationReference.size() + 3;
				}
			}

			// Append the remainder of the line to the line string
			formattedLogLine = formattedLogLine + std::string(searchStart, logLine.cend());

			// Print the formatted line
			Debug::log_error() << formattedLogLine << Debug::end;
			result << formattedLogLine << std::endl;

			// Print the small quick-peak area
			if (printPeakWindows && errorLineNo != -1)
			{
				// Prefix string
				std::string prefix = std::string(errorPrefixSize, ' ');

				Debug::log_error() << prefix << "at: " << Debug::end;
				result << prefix << "at: " << std::endl;

				std::stringstream ssl(source);
				std::string line;
				for (int i = 1; std::getline(ssl, line); ++i)
				{
					// Only print the lines in the area of interest
					if (i < errorLineNo - printAreaSize)
					{
						continue;
					}
					else if (i > errorLineNo + printAreaSize)
					{
						break;
					}

					// Print the prefix
					Debug::log_error() << prefix;
					result << prefix;

					if (i == errorLineNo)
					{
						Debug::log_error() << "--> \"";
						result << "--> \"";
					}
					else
					{
						Debug::log_error() << "    \"";
						result << "    \"";
					}

					// Print the current line and end the line
					Debug::log_error() << line << "\"" << Debug::end;
					result << line << "\"" << std::endl;
				}
			}
		}

		// Return the resulting error string
		return result.str();
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string detailedShaderErrorLog(std::array<std::pair<GLuint, std::string>, 6> const& sources, std::string const& log, int printAreaSize = 2, bool printPeakWindows = false)
	{
		// Various shader references
		static const std::regex s_infoHeaders[] =
		{
			std::regex("Tessellation control info"),
			std::regex("Tessellation evaluation info"),
			std::regex("Vertex info"),
			std::regex("Geometry info"),
			std::regex("Fragment info"),
			std::regex("Compute info"),
		};

		// Create a string stream from the log message to process each line
		std::stringstream ss(log);

		// Create a string stream for the result
		std::stringstream result;

		// Temporary buffer holding the current error chunk
		std::stringstream currentInfo;
		int currentShaderIndex = -1;

		// Process each line
		std::smatch capture;
		for (std::string logLine; std::getline(ss, logLine);)
		{
			// Look for a header reference
			for (size_t i = 0; i < 6; ++i)
			{
				if (std::regex_search(logLine, capture, s_infoHeaders[i]))
				{
					// Print the current log, if any
					if (currentShaderIndex == -1)
					{
						result << detailedShaderFileErrorLog("", currentInfo.str(), printPeakWindows) << std::endl;
						currentInfo.str("");
					}
					else if (currentShaderIndex >= 0)
					{
						result << detailedShaderFileErrorLog(sources[currentShaderIndex].second, currentInfo.str(), printPeakWindows) << std::endl;
						currentInfo.str("");
					}

					// Store the new index
					currentShaderIndex = i;
				}
			}
			
			// Append the line
			currentInfo << logLine << std::endl;
		}

		// Print the last entry
		if (currentShaderIndex == -1)
		{
			result << detailedShaderFileErrorLog("", currentInfo.str(), printPeakWindows) << std::endl;
		}
		else if (currentShaderIndex >= 0)
		{
			result << detailedShaderFileErrorLog(sources[currentShaderIndex].second, currentInfo.str(), printPeakWindows) << std::endl;
		}

		// Return the result
		return result.str();
	}

	////////////////////////////////////////////////////////////////////////////////
	std::pair<GLuint, std::string> loadShaderFile(Scene::Scene& scene, GLenum shaderType, const std::string& shaderName, const std::string& fileName, 
		ShaderParameters const& shaderParameters)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, fileName);

		Debug::log_trace() << "Loading shader file: " << fileName << Debug::end;

		// Assemble the full shader source
		std::string fullShaderSource = getShaderSource(scene, shaderType, shaderName, fileName, shaderParameters);

		// Make sure it exists.
		if (fullShaderSource.empty())
			return std::make_pair<GLuint, std::string>(0, "");

		// Create the shader object.
		GLuint shader = glCreateShader(shaderType);

		// Write to disk the the compiled shader source
		std::string relativeFilepath = std::filesystem::relative(fileName, EnginePaths::openGlShadersFolder()).string();
		std::string fullShaderOutputPath = (EnginePaths::generatedFilesFolder() / "Shaders" / "Output" / "OpenGL" / relativeFilepath).string();

		//Debug::log_debug() << "Saving generated shader source to: " << fullShaderOutputPath << Debug::end;
		//EnginePaths::makeDirectoryStructure(fullShaderOutputPath, true);
		//std::ofstream fullShaderOutputFile(fullShaderOutputPath);
		//fullShaderOutputFile << fullShaderSource << std::endl;

		// Attach the source
		const GLchar* sourcePtrs[] = { fullShaderSource.c_str() };
		glShaderSource(shader, 1, sourcePtrs, NULL);

		// Compile it.
		glCompileShader(shader);

		// Make sure it compiled successfully
		GLint status;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

		//fullShaderOutputFile << std::string(80, '=') << std::endl;
		if (status == GL_FALSE)
		{
			Debug::log_error() << "Error during shader compilation while loading shader file: " << fileName << Debug::end;

			static GLchar s_buffer[4098];
			glGetShaderInfoLog(shader, sizeof(s_buffer) / sizeof(GLchar), NULL, s_buffer);
			std::string detailedError = detailedShaderFileErrorLog(fullShaderSource, s_buffer);
			Debug::log_debug() << "Raw error info: " << Debug::end << std::string(s_buffer) << Debug::end;

			//fullShaderOutputFile << std::string(4, ' ') << "COMPILE STATUS = FALSE" << std::endl;
			//fullShaderOutputFile << std::string(80, '-') << std::endl;
			//fullShaderOutputFile << "Compilation log: " << std::endl;
			//fullShaderOutputFile << detailedError << std::endl;
			//fullShaderOutputFile << std::string(80, '-') << std::endl;
			//fullShaderOutputFile << "Raw error log: " << std::endl;
			//fullShaderOutputFile << s_buffer << std::endl;
		}
		else
		{
			Debug::log_trace() << "Shader file successfully loaded: " << fileName << Debug::end;

			//fullShaderOutputFile << std::string(4, ' ') << "COMPILE STATUS = TRUE" << std::endl;
		}
		//fullShaderOutputFile << std::string(80, '=') << std::endl;

		//fullShaderOutputFile.close();

		return std::make_pair(shader, fullShaderSource);
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string getShaderName(std::string const& folderName, std::string const& fileName, std::string const& parameters)
	{
		std::stringstream ss;
		ss << folderName << "/" << fileName << parameters;
		return ss.str();
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string getShaderFileName(std::string const& folderName, std::string const& fileName, std::string const& fileTypeSuffix)
	{
		return (EnginePaths::openGlShadersFolder() / folderName / (fileName + "_" + fileTypeSuffix + ".glsl")).string();
	}

	////////////////////////////////////////////////////////////////////////////////
	bool loadShader(Scene::Scene& scene, const std::string& folderName, const std::string& fileName, 
		const std::string& customShaderName, ShaderParameters const& shaderParameters)
	{
		std::string shaderName = customShaderName.empty() == false ? customShaderName : getShaderName(folderName, fileName);

		if (scene.m_shaders.find(shaderName) != scene.m_shaders.end())
			return true;

		Profiler::ScopedCpuPerfCounter perfCounter(scene, shaderName);

		Debug::log_trace() << "Loading shader: " << shaderName << Debug::end;

		// Generate the generated shaders first
		Asset::generateGeneratedShaders(scene);

		// Load the shader files.
		static std::array<std::pair<GLenum, std::string>, 6> shaderTypes =
		{
			std::make_pair(GL_TESS_CONTROL_SHADER, "tcs"),
			std::make_pair(GL_TESS_EVALUATION_SHADER, "tes"),
			std::make_pair(GL_VERTEX_SHADER, "vs"),
			std::make_pair(GL_GEOMETRY_SHADER, "gs"),
			std::make_pair(GL_FRAGMENT_SHADER, "fs"),
			std::make_pair(GL_COMPUTE_SHADER, "cs"),
		};

		// Load the shader files.
		std::array<std::pair<GLuint, std::string>, 6> shaders;

		// Create the program and attach the shaders.
		GLuint program = glCreateProgram();

		// Associate the proper label to it
		glObjectLabel(GL_PROGRAM, program, shaderName.length(), shaderName.c_str());

		for (size_t i = 0; i < shaderTypes.size(); ++i)
		{
			std::string const& filePath = getShaderFileName(folderName, fileName, shaderTypes[i].second);
			//Debug::log_debug() << "Trying to load shader source file: " << filePath << Debug::end;

			if (existsFile(filePath))
			{
				Debug::log_debug() << "Attaching shader: " << filePath << Debug::end;
				shaders[i] = loadShaderFile(scene, shaderTypes[i].first, shaderName, filePath, shaderParameters);
				glAttachShader(program, shaders[i].first);
			}
		}

		// Link the program.
		glLinkProgram(program);

		// Detach and releases the shader objects that are no longer needed.
		for (auto const& shader: shaders)
		{
			if (shader.first != 0)
			{
				glDetachShader(program, shader.first);
				glDeleteShader(shader.first);
			}
		}

		// Write to disk the the program state
		std::string fullShaderOutputPath = (EnginePaths::generatedFilesFolder() / "Shaders" / "Output" / "OpenGL" / shaderName).string();

		EnginePaths::makeDirectoryStructure(fullShaderOutputPath, true);
		std::ofstream fullShaderOutputFile(fullShaderOutputPath);
		fullShaderOutputFile << std::string(80, '=') << std::endl;

		// Make sure it linked successfully.
		GLint status;
		glGetProgramiv(program, GL_LINK_STATUS, &status);

		if (status == GL_FALSE)
		{
			Debug::log_error() << "Error during program compilation while loading shader: " << shaderName << Debug::end;

			static GLchar s_buffer[4098];
			glGetProgramInfoLog(program, sizeof(s_buffer) / sizeof(GLchar), NULL, s_buffer);
			std::string detailedError = detailedShaderErrorLog(shaders, s_buffer);
			Debug::log_debug() << "Raw error info: " << Debug::end << std::string(s_buffer) << Debug::end;

			fullShaderOutputFile << std::string(4, ' ') << "COMPILE STATUS = FALSE" << std::endl;
			fullShaderOutputFile << std::string(80, '-') << std::endl;
			fullShaderOutputFile << "Compilation log: " << std::endl;
			fullShaderOutputFile << detailedError << std::endl;
			fullShaderOutputFile << std::string(80, '-') << std::endl;
			fullShaderOutputFile << "Raw error log: " << std::endl;
			fullShaderOutputFile << s_buffer << std::endl;
			fullShaderOutputFile << std::string(80, '=') << std::endl;

			return false;
		}
		else
		{
			fullShaderOutputFile << std::string(4, ' ') << "COMPILE STATUS = TRUE" << std::endl;
			fullShaderOutputFile << std::string(80, '=') << std::endl;
		}

		// Create the shader object and store it.
		GPU::Shader shader;
		shader.m_program = program;

		scene.m_shaders[shaderName] = shader;

		Debug::log_trace() << "Shader loaded successfully: " << shaderName << Debug::end;

		return true;
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	std::string enumToShaderDefines(T enumMeta)
	{
		std::stringstream sstr;

		for (auto const& it : enumMeta.members)
		{
			sstr << "#define " << it.name << " " << std::to_string(it.value) << std::endl;
		}

		return sstr.str();
	}

	////////////////////////////////////////////////////////////////////////////////
	void generateGeneratedShaders(Scene::Scene& scene)
	{
		Debug::log_trace() << "Generating generated shaders" << Debug::end;

		// Resource indices
		{
			std::string filePath = "Shaders/OpenGL/Generated/resource_indices.glsl";
			if (scene.m_textFiles.find(filePath) == scene.m_textFiles.end())
			{
				Debug::log_debug() << filePath << Debug::end;

				std::stringstream sstr;

				sstr << "// Vertex indices" << std::endl;
				sstr << enumToShaderDefines(GPU::VertexAttribIndices_meta) << std::endl;

				sstr << "// UBO indices" << std::endl;
				sstr << enumToShaderDefines(GPU::UniformBufferIndices_meta) << std::endl;

				sstr << "// Texture indices" << std::endl;
				sstr << enumToShaderDefines(GPU::TextureIndices_meta) << std::endl;

				storeTextFile(scene, filePath, sstr.str());
			}
		}

		// Capabilities
		{
			std::string filePath = "Shaders/OpenGL/Generated/capabilities.glsl";
			if (scene.m_textFiles.find(filePath) == scene.m_textFiles.end())
			{
				Debug::log_debug() << filePath << Debug::end;

				std::stringstream sstr;

				sstr << "// Capabilities" << std::endl;
				for (auto const& capabilityCategory : GPU::s_capabilityAttribs)
				for (auto const& capabilityIt : capabilityCategory.second)
				{
					auto const& [capabilityId, capability] = capabilityIt;

					if (capability.m_values.size() <= 1)
					{
						sstr << "#define CAP_" << GPU::enumToString(capability.m_enum) << " " << std::to_string(capability.m_values[0]) << std::endl;
					}
					else
					{
						for (size_t i = 0; i < capability.m_values.size(); ++i)
						{
							sstr << "#define CAP_" << GPU::enumToString(capability.m_enum) << "_" + std::to_string(i) + " " << std::to_string(capability.m_values[i]) << std::endl;
						}
					}
				}

				storeTextFile(scene, filePath, sstr.str());
			}
		}

		// Extensions
		{
			std::string filePath = "Shaders/OpenGL/Generated/extensions.glsl";
			if (scene.m_textFiles.find(filePath) == scene.m_textFiles.end())
			{
				Debug::log_debug() << filePath << Debug::end;

				GLint n;
				glGetIntegerv(GL_NUM_EXTENSIONS, &n);

				std::stringstream sstr;

				sstr << "// Extensions" << std::endl;
				if (GPU::enableOptionalExtensions())
				{
					for (GLint i = 0; i < n; i++)
					{
						sstr << "#define EXT_" << glGetStringi(GL_EXTENSIONS, i) << " 1" << std::endl;
					}
				}

				storeTextFile(scene, filePath, sstr.str());
			}
		}
		
		Debug::log_trace() << "Successfully generated generated shaders" << Debug::end;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool loadDefaultFont(Scene::Scene& scene)
	{
		std::string fontName = "Default";

		Profiler::ScopedCpuPerfCounter perfCounter(scene, fontName);

		// Make sure it isn't loaded already.
		if (scene.m_fonts.find(fontName) != scene.m_fonts.end())
			return true;

		Debug::log_trace() << "Loading default font" << Debug::end;

		// Load the font
		scene.m_fonts[fontName] = ImGui::GetIO().Fonts->AddFontDefault();

		Debug::log_trace() << "Successfully loaded default font" << Debug::end;
		return true;
	}

	////////////////////////////////////////////////////////////////////////////////
	const ImWchar* getFontGlyphRanges(Scene::Scene& scene)
	{
		static ImVector<ImWchar> s_ranges;

		if (s_ranges.empty())
		{
			// Default characters
			ImFontGlyphRangesBuilder glyphBuilder;
			glyphBuilder.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesDefault());
			
			// Try to approximate some of the characters using the day and month names of the current locale
			char buffer[80];
			struct tm timeinfo;
			for (size_t i = 0; i < 12; ++i)
			{
				timeinfo.tm_mon = i;
				strftime(buffer, sizeof(buffer), "%B", &timeinfo);
				glyphBuilder.AddText(buffer);
			}
			for (size_t i = 0; i < 7; ++i)
			{
				timeinfo.tm_wday = i;
				strftime(buffer, sizeof(buffer), "%A", &timeinfo);
				glyphBuilder.AddText(buffer);
			}
			glyphBuilder.BuildRanges(&s_ranges);
		}

		return s_ranges.Data;
	}

	////////////////////////////////////////////////////////////////////////////////
	bool loadExternalFont(Scene::Scene& scene, std::string const& fontName, std::string const& fontPath, int fontSize)
	{
		Profiler::ScopedCpuPerfCounter perfCounter(scene, fontName);

		// Make sure it isn't loaded already.
		if (scene.m_fonts.find(fontName) != scene.m_fonts.end())
			return true;

		Debug::log_trace() << "Loading external font " << fontName << Debug::end;

		// Look for the font in the assets folder first
		std::vector<std::string> fullFontPaths =
		{
			(EnginePaths::assetsFolder() / fontPath).string(),
			(EnginePaths::systemFontsFolder() / fontPath).string()
		};

		// Look for the first existing font path
		std::string fullFontPath;
		for (size_t i = 0; i < fullFontPaths.size() && fullFontPath.empty(); ++i)
			if (std::filesystem::exists(std::filesystem::path(fullFontPaths[i])))
				fullFontPath = fullFontPaths[i];

		// Early out if the font wasn't found
		if (fullFontPath.empty())
		{
			Debug::log_error() << "Unable to load external font" << fontName << Debug::end;
			return false;
		}

		// Load the font
		scene.m_fonts[fontName] = ImGui::GetIO().Fonts->AddFontFromFileTTF(fullFontPath.c_str(), fontSize, nullptr, getFontGlyphRanges(scene));

		// Append the custom glyphs
		static const ImWchar icons_ranges[] = { ICON_MIN_IGFD, ICON_MAX_IGFD, 0 };
		ImFontConfig icons_config;
		icons_config.MergeMode = true;
		icons_config.PixelSnapH = true;
		ImGui::GetIO().Fonts->AddFontFromMemoryCompressedBase85TTF(FONT_ICON_BUFFER_NAME_IGFD, 32.0f, &icons_config, icons_ranges);

		Debug::log_trace() << "Successfully loaded external font " << fontName << Debug::end;

		// Success
		return true;
	}
}