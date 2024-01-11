#pragma once

////////////////////////////////////////////////////////////////////////////////
//  Headers
////////////////////////////////////////////////////////////////////////////////

#include "PCH.h"
#include "Common.h"

////////////////////////////////////////////////////////////////////////////////
/// ASSET LOADING FUNCTIONS
////////////////////////////////////////////////////////////////////////////////
namespace Asset
{
	////////////////////////////////////////////////////////////////////////////////
	struct TextFile
	{
		TextFile();
		TextFile(std::string const& contents);
		operator std::string() const;

		std::string m_contents;
		std::filesystem::file_time_type m_lastModified;
	};

	////////////////////////////////////////////////////////////////////////////////
	bool makeDirectoryStructure(std::string const& fileName);

	////////////////////////////////////////////////////////////////////////////////
	bool existsFile(std::filesystem::path const& filePath);

	////////////////////////////////////////////////////////////////////////////////
	bool existsFile(std::string const& fileName);

	////////////////////////////////////////////////////////////////////////////////
	void storeTextFile(Scene::Scene& scene, const std::string& fileName, std::string const& contents);

	////////////////////////////////////////////////////////////////////////////////
	std::optional<TextFile> loadTextFile(Scene::Scene& scene, const std::string& fileName, bool allowCaching = true);

	////////////////////////////////////////////////////////////////////////////////
	bool saveTextFile(Scene::Scene& scene, const std::string& fileName, const std::string& contents);

	////////////////////////////////////////////////////////////////////////////////
	bool saveTextFile(Scene::Scene& scene, const std::string& fileName, const std::stringstream& contents);

	////////////////////////////////////////////////////////////////////////////////
	std::optional<std::ordered_pairs_in_blocks> loadPairsInBlocks(Scene::Scene& scene, const std::string& fileName);

	////////////////////////////////////////////////////////////////////////////////
	bool savePairsInBlocks(Scene::Scene& scene, const std::string& fileName, const std::ordered_pairs_in_blocks& pairs, bool alignCols = true);

	////////////////////////////////////////////////////////////////////////////////
	bool savePairsInBlocks(Scene::Scene& scene, const std::string& fileName, const std::unordered_pairs_in_blocks& pairs, bool alignCols = true);

	////////////////////////////////////////////////////////////////////////////////
	bool saveCsv(Scene::Scene& scene, std::string const& fileName, std::vector<std::vector<std::string>> const& contents, std::string const& separator = ";", std::vector<std::string> const& headers = std::vector<std::string>());

	////////////////////////////////////////////////////////////////////////////////
	std::optional<cv::Mat> loadImage(Scene::Scene& scene, const std::string& filePath);

	////////////////////////////////////////////////////////////////////////////////
	bool loadTexture(Scene::Scene& scene, const std::string& textureName, const std::string& filePath);

	////////////////////////////////////////////////////////////////////////////////
	bool load3DTexture(Scene::Scene& scene, const std::string& textureName, const std::string& filePath);

	////////////////////////////////////////////////////////////////////////////////
	bool loadCubeMap(Scene::Scene& scene, const std::string& textureName, const std::string& filePath, const std::string& leftName = "left.tga", const std::string& rightName = "right.tga",
		const std::string& topName = "top.tga", const std::string& bottomName = "bottom.tga", const std::string& backName = "back.tga", const std::string& frontName = "front.tga");

	////////////////////////////////////////////////////////////////////////////////
	bool saveImage(Scene::Scene& scene, std::string filePath, size_t width, size_t height, size_t channels, unsigned char* pixels, bool flipUD = false);

	////////////////////////////////////////////////////////////////////////////////
	bool saveImage(Scene::Scene& scene, std::string filePath, size_t width, size_t height, size_t channels, float* pixels, bool flipUD = false);

	////////////////////////////////////////////////////////////////////////////////
	bool saveImage(Scene::Scene& scene, std::string filePath, cv::Mat const& image, bool flipUD = false);

	////////////////////////////////////////////////////////////////////////////////
	bool saveImage(Scene::Scene& scene, std::string filePath, Eigen::MatrixXd const& image, bool flipUD = false);

	////////////////////////////////////////////////////////////////////////////////
	bool saveImage(Scene::Scene& scene, std::string filePath, Eigen::MatrixXf const& image, bool flipUD = false);

	////////////////////////////////////////////////////////////////////////////////
	bool saveImage(Scene::Scene& scene, std::string filePath, Eigen::MatrixX<unsigned char> const& image, bool flipUD = false);

	////////////////////////////////////////////////////////////////////////////////
	bool saveTexture(Scene::Scene& scene, const std::string& textureName, std::string filePath);

	////////////////////////////////////////////////////////////////////////////////
	float linearizeDepth(float depth, float near, float far);

	////////////////////////////////////////////////////////////////////////////////
	float reconstructCameraZ(float depth, glm::mat4 projection);

	////////////////////////////////////////////////////////////////////////////////
	void linearizeDepthImage(unsigned char* pixels, int width, int height, float near, float far);

	////////////////////////////////////////////////////////////////////////////////
	bool saveDefaultFramebuffer(Scene::Scene& scene, std::string filePath, int width, int height);

	////////////////////////////////////////////////////////////////////////////////
	bool saveGbufferColor(Scene::Scene& scene, std::string filePath, int gbufferId, int layerId, int width, int height);

	////////////////////////////////////////////////////////////////////////////////
	bool saveGbufferNormal(Scene::Scene& scene, std::string filePath, int gbufferId, int layerId, int width, int height);

	////////////////////////////////////////////////////////////////////////////////
	bool saveGbufferLinearDepth(Scene::Scene& scene, std::string filePath, int gbufferId, int layerId, int width, int height, float near, float far);

	////////////////////////////////////////////////////////////////////////////////
	bool saveGbuffer(Scene::Scene& scene, const std::string& filePath, int gbufferId, int numLayers, int width, int height, float near, float far);

	////////////////////////////////////////////////////////////////////////////////
	bool loadMesh(Scene::Scene& scene, const std::string& filePath);

	////////////////////////////////////////////////////////////////////////////////
	bool loadTfModel(Scene::Scene& scene, const std::string& modelName, TensorFlow::ModelSpec const& modelSpec = {});

	////////////////////////////////////////////////////////////////////////////////
	template<typename M>
	std::vector<std::string> generateMetaEnumDefines(std::vector<std::string> result, M const& meta)
	{
		result.reserve(meta.members.size() + 1);
		result.push_back("ENUM_" + std::string(meta.name) + "_DEFINED 1");
		std::transform(meta.members.begin(), meta.members.end(), std::back_inserter(result), [&](auto member)
			{
				return std::string(meta.name) + "_" + std::string(member.name) + " " + std::to_string(member.value);
			});
		return result;
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename M>
	std::vector<std::string> generateMetaEnumDefines(M const& meta)
	{
		return generateMetaEnumDefines(std::vector<std::string>(), meta);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename M0, typename... Ms>
	std::vector<std::string> generateMetaEnumDefines(std::vector<std::string> result, const M0& meta0, const Ms&... metas)
	{
		return generateMetaEnumDefines(generateMetaEnumDefines(result, metas...), meta0);
	}

	////////////////////////////////////////////////////////////////////////////////
	template<typename M0, typename... Ms>
	std::vector<std::string> generateMetaEnumDefines(const M0& meta0, const Ms&... metas)
	{
		return generateMetaEnumDefines(std::vector<std::string>(), meta0, metas...);
	}

	////////////////////////////////////////////////////////////////////////////////
	std::string generateShaderDefines(const std::vector<std::string>& defines);

	////////////////////////////////////////////////////////////////////////////////
	std::string getShaderName(std::string const& folderName, std::string const& fileName, std::string const& parameters = "");

	////////////////////////////////////////////////////////////////////////////////
	struct ShaderParameters
	{
		std::vector<std::string> m_defines;
		std::vector<std::string> m_enums;
		std::vector<std::string> m_structs;
	};

	////////////////////////////////////////////////////////////////////////////////
	bool loadShader(Scene::Scene& scene, const std::string& folderName, const std::string& fileName,
		const std::string& customShaderName = "", ShaderParameters const& shaderParameters = {});

	////////////////////////////////////////////////////////////////////////////////
	void generateGeneratedShaders(Scene::Scene& scene);

	////////////////////////////////////////////////////////////////////////////////
	bool loadDefaultFont(Scene::Scene& scene);

	////////////////////////////////////////////////////////////////////////////////
	bool loadExternalFont(Scene::Scene& scene, std::string const& fontName, std::string const& fontPath, int fontSize);
}