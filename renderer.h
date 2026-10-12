// minimalistic code to draw a single triangle, this is not part of the API.
#include "shaderc/shaderc.h" // needed for compiling shaders at runtime
#include "FileIntoString.h"
#include "Camera.h"
#include <vector>
#ifdef _WIN32 // must use MT platform DLL libraries on windows
#pragma comment(lib, "shaderc_combined.lib") 
#endif

void PrintLabeledDebugString(const char* label, const char* toPrint)
{
	std::cout << label << toPrint << std::endl;

//OutputDebugStringA is a windows-only function 
#if defined WIN32 
	OutputDebugStringA(label);
	OutputDebugStringA(toPrint);
#endif
}

class Renderer
{
	// proxy handles
	GW::SYSTEM::GWindow win;
	GW::GRAPHICS::GVulkanSurface vlk;
	VkRenderPass renderPass;
	GW::CORE::GEventReceiver shutdown;
	
	// Input proxy handles
	GW::INPUT::GInput keyboard; // For keyboard and mouse input
	GW::INPUT::GController controller; // For game controller input

	// what we need at a minimum to draw a triangle
	VkDevice device = nullptr;
	VkPhysicalDevice physicalDevice = nullptr;
	VkBuffer vertexHandle = nullptr;
	VkDeviceMemory vertexData = nullptr;
	VkShaderModule vertexShader = nullptr;
	VkShaderModule fragmentShader = nullptr;
	VkPipeline pipeline = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;

	unsigned int windowWidth, windowHeight;

	// TODO: Part 1c
	struct mVertex { float x, y, z, w; };
	// TODO: Part 2a
	GW::MATH::GMATRIXF worldMatrices[6];
	GW::MATH::GMatrix matrix;
	// TODO: Part 2b
	struct SHADER_VARS
	{
		GW::MATH::GMATRIXF world[6];
		GW::MATH::GMATRIXF view;
		GW::MATH::GMATRIXF projection;
	};
	// TODO: Part 3a
	GW::MATH::GMATRIXF viewMatrix;
	// TODO: Part 3f 
	// TODO: Part 2c // TODO: Part 4y
	// Uniform buffer for shader data
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	// TODO: Part 2e
	// Descriptor set layout for uniform buffer
	VkDescriptorSetLayout descriptorSetLayout = nullptr;
	// TODO: Part 2f
	VkDescriptorPool descriptorPool = nullptr;
	// TODO: Part 2g
	std::vector<VkDescriptorSet> descriptorSets;
	// TODO: Part 3c
	GW::MATH::GMATRIXF projectionMatrix; // For perspective projection
	// TODO: Part 3d
	// TODO: Part 4a

public:
	Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GVulkanSurface _vlk)
	{
		win = _win;
		vlk = _vlk;

		// Create input proxies
		keyboard.Create(win);
		controller.Create();

		// TODO: Part 2a
		// TODO: Part 2e
		// TODO: Part 3a
		// TODO: Part 3c
		// TODO: Part 3d
		// TODO: Part 4a
		matrix.Create();
		InitializeWorldMatrix();
		InitializeViewMatrix();
		InitializeProjectionMatrix();

		UpdateWindowDimensions();
		InitializeGraphics();
		BindShutdownCallback();
	}

private:
	void UpdateWindowDimensions()
	{
		win.GetClientWidth(windowWidth);
		win.GetClientHeight(windowHeight);
	}

	void InitializeGraphics()
	{
		GetHandlesFromSurface();
		InitializeVertexBuffer();
		// TODO: Part 2d
		// TODO: Part 2f // TODO: Part 4y
		// TODO: Part 2g // TODO: Part 4y
		// TODO: Part 2h // TODO: Part 4y
		CreateDescriptorSetLayout();
		InitializeUniformBuffers(); 
		CreateDescriptorPool(); 
		CreateDescriptorSets();
		CompileShaders();
		InitializeGraphicsPipeline();
	}

	void GetHandlesFromSurface()
	{
		vlk.GetDevice((void**)&device);
		vlk.GetPhysicalDevice((void**)&physicalDevice);
		vlk.GetRenderPass((void**)&renderPass);
	}
	
	void UpdateViewMatrix(float deltaTime)
	{
		viewMatrix = FreeLookCamera(win);
	}

	void InitializeProjectionMatrix()
	{
		// Get the window's aspect ratio
		float aspectRatio;
		unsigned int width, height;
		win.GetClientWidth(width);
		win.GetClientHeight(height);
		aspectRatio = static_cast<float>(width) / static_cast<float>(height);

		// Create a left-handed perspective projection for Vulkan
		// vertical FOV = 65 degrees, near plane = 0.1, far plane = 100
		float fovInRadians = G_DEGREE_TO_RADIAN_F(65.0f);

		// Use the ProjectionVulkanLHF function from the matrix library
		matrix.ProjectionVulkanLHF(fovInRadians, aspectRatio, 0.1f, 100.0f, projectionMatrix);
	}

	// Helper function to initialize the world matrix
	void InitializeWorldMatrix()
	{
		// Create an matrix proxy
		matrix.Create();

		// Floor (original implementation)
		// Create a rotation matrix around the X-axis (90 degrees)
		GW::MATH::GMATRIXF rotationX;
		matrix.RotateXLocalF(GW::MATH::GIdentityMatrixF, G_DEGREE_TO_RADIAN_F(90.0f), rotationX);

		// Create a translation matrix down the Y-axis (0.5f)
		GW::MATH::GMATRIXF translationY;
		GW::MATH::GVECTORF translationVector = { 0.0f, -0.5f, 0.0f, 1.0f };
		matrix.TranslateLocalF(GW::MATH::GIdentityMatrixF, translationVector, translationY);

		// Combine the rotation and translation matrices for the floor
		matrix.MultiplyMatrixF(rotationX, translationY, worldMatrices[0]);

		// Ceiling - Similar to floor but with different translation
		GW::MATH::GMATRIXF ceilingRotation;
		matrix.RotateXLocalF(GW::MATH::GIdentityMatrixF, G_DEGREE_TO_RADIAN_F(-90.0f), ceilingRotation);

		GW::MATH::GMATRIXF ceilingTranslation;
		GW::MATH::GVECTORF ceilingTranslationVector = { 0.0f, 0.5f, 0.0f, 1.0f };
		matrix.TranslateLocalF(GW::MATH::GIdentityMatrixF, ceilingTranslationVector, ceilingTranslation);

		matrix.MultiplyMatrixF(ceilingRotation, ceilingTranslation, worldMatrices[1]);

		// Left Wall
		GW::MATH::GMATRIXF leftWallRotationZ;
		matrix.RotateYLocalF(GW::MATH::GIdentityMatrixF, G_DEGREE_TO_RADIAN_F(90.0f), leftWallRotationZ);

		GW::MATH::GMATRIXF leftWallTranslation;
		GW::MATH::GVECTORF leftWallTranslationVector = { -0.5f, 0.0f, 0.0f, 1.0f };
		matrix.TranslateLocalF(GW::MATH::GIdentityMatrixF, leftWallTranslationVector, leftWallTranslation);

		matrix.MultiplyMatrixF(leftWallRotationZ, leftWallTranslation, worldMatrices[2]);

		// Right Wall
		GW::MATH::GMATRIXF rightWallRotationZ;
		matrix.RotateYLocalF(GW::MATH::GIdentityMatrixF, G_DEGREE_TO_RADIAN_F(-90.0f), rightWallRotationZ);

		GW::MATH::GMATRIXF rightWallTranslation;
		GW::MATH::GVECTORF rightWallTranslationVector = { 0.5f, 0.0f, 0.0f, 1.0f };
		matrix.TranslateLocalF(GW::MATH::GIdentityMatrixF, rightWallTranslationVector, rightWallTranslation);

		matrix.MultiplyMatrixF(rightWallRotationZ, rightWallTranslation, worldMatrices[3]);

		// Back Wall
		GW::MATH::GMATRIXF backWallRotationX;
		matrix.RotateXLocalF(GW::MATH::GIdentityMatrixF, G_DEGREE_TO_RADIAN_F(0.0f), backWallRotationX);

		GW::MATH::GMATRIXF backWallTranslation;
		GW::MATH::GVECTORF backWallTranslationVector = { 0.0f, 0.0f, 0.5f, 1.0f };
		matrix.TranslateLocalF(GW::MATH::GIdentityMatrixF, backWallTranslationVector, backWallTranslation);

		matrix.MultiplyMatrixF(backWallRotationX, backWallTranslation, worldMatrices[4]);

		// Front Wall
		GW::MATH::GMATRIXF frontWallRotationY;
		matrix.RotateYLocalF(GW::MATH::GIdentityMatrixF, G_DEGREE_TO_RADIAN_F(180.0f), frontWallRotationY);

		GW::MATH::GMATRIXF frontWallTranslation;
		GW::MATH::GVECTORF frontWallTranslationVector = { 0.0f, 0.0f, -0.5f, 1.0f };
		matrix.TranslateLocalF(GW::MATH::GIdentityMatrixF, frontWallTranslationVector, frontWallTranslation);

		matrix.MultiplyMatrixF(frontWallRotationY, frontWallTranslation, worldMatrices[5]);
	}

	// New function to initialize the view matrix
	void InitializeViewMatrix()
	{
		// Create camera position vector - placed above, behind and to the right of the scene
		GW::MATH::GVECTORF cameraPosition = { 0.25f, -0.125f, -0.25f, 1.0f };

		// Define the target point (origin)
		GW::MATH::GVECTORF targetPosition = { 0.0f, -0.5f, 0.0f, 1.0f };

		// Define the up vector for the camera
		GW::MATH::GVECTORF upVector = { 0.0f, 0.5f, 0.0f, 0.0f };

		// Create view matrix using LookAt function
		GW::MATH::GMATRIXF viewMatrix;
		matrix.LookAtLHF(cameraPosition, targetPosition, upVector, viewMatrix);

		// Store the view matrix
		// You'll need to add a viewMatrix member to your Renderer class
		this->viewMatrix = viewMatrix;

		// Create a combined world-view matrix for the shader
		GW::MATH::GMATRIXF worldViewMatrix;
		matrix.MultiplyMatrixF(worldMatrices[0], viewMatrix, worldViewMatrix);
	}

	void CreateDescriptorSets() {
		unsigned int maxFramesInFlight;
		vlk.GetSwapchainImageCount(maxFramesInFlight);

		descriptorSets.resize(maxFramesInFlight);

		std::vector<VkDescriptorSetLayout> layouts(maxFramesInFlight, descriptorSetLayout);

		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = maxFramesInFlight;
		allocInfo.pSetLayouts = layouts.data();

		if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate descriptor sets!");
		}

		// Initialize all descriptor sets
		for (unsigned int i = 0; i < maxFramesInFlight; i++) {
			UpdateDescriptorSet(i);
		}
	}

	// Update descriptor sets to link with uniform buffer
	void UpdateDescriptorSet(unsigned int index) {
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = uniformBuffers[index];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(SHADER_VARS);

		VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSets[index];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
	}


	// Function to create the descriptor pool
	void CreateDescriptorPool()
	{
		// Get the number of swapchain images to determine how many descriptor sets we need
		unsigned int maxFramesInFlight;
		vlk.GetSwapchainImageCount(maxFramesInFlight);

		// Describe the type of descriptor sets we'll create from this pool
		VkDescriptorPoolSize poolSize = {};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // Using uniform buffers
		poolSize.descriptorCount = maxFramesInFlight; // One descriptor per frame

		// Setup the descriptor pool create info
		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1; // Just one type of descriptor
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = maxFramesInFlight; // Match the number of frames in flight
		poolInfo.flags = 0; // No special flags needed

		// Create the descriptor pool
		if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create descriptor pool!");
		}
	}

	void CreateDescriptorSetLayout()
	{
		// Create binding for our uniform buffer
		VkDescriptorSetLayoutBinding layoutBinding = {};
		layoutBinding.binding = 0;                                   // Binding point in shader
		layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // Uniform buffer
		layoutBinding.descriptorCount = 1;                           // Just one descriptor
		layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; // Used in both vertex and fragment shaders
		layoutBinding.pImmutableSamplers = nullptr;                  // Not using samplers

		// Create the descriptor set layout with our binding
		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;                                // Just one binding
		layoutInfo.pBindings = &layoutBinding;                      // Pointer to the binding

		// Create the descriptor set layout
		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) 
		{
			throw std::runtime_error("Failed to create descriptor set layout!");
		}
	}

	void InitializeUniformBuffers()
	{
		// Get the number of swapchain images
		unsigned int maxFramesInFlight = 0;
		vlk.GetSwapchainImageCount(maxFramesInFlight);

		// Resize the uniform buffers vector to hold the correct number of buffers
		uniformBuffers.resize(maxFramesInFlight);
		uniformBuffersMemory.resize(maxFramesInFlight);

		// Create a buffer for each frame
		for (int i = 0; i < maxFramesInFlight; ++i)
		{
			// Create a uniform buffer for each frame
			GvkHelper::create_buffer(physicalDevice, device,
				sizeof(SHADER_VARS), 
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				&uniformBuffers[i], &uniformBuffersMemory[i]);

			// Initialize the uniform buffer with the world matrix data
			SHADER_VARS shaderData = {};
			// Copy all world matrices to the shader data
			for (int j = 0; j < 6; j++) {
				shaderData.world[j] = worldMatrices[j];
			}
			shaderData.view = viewMatrix;   // Set the view matrix
			shaderData.projection = projectionMatrix;

			GvkHelper::write_to_buffer(device, uniformBuffersMemory[i], &shaderData, sizeof(SHADER_VARS)); // Transfer data to the uniform buffer
		}
	}

	// Helper function to create grid vertices
	void CreateGridVertices(std::vector<mVertex>& outVertices, int gridDensity, float gridExtent)
	{
		// Clear any existing vertices
		outVertices.clear();

		// Calculate the step size based on the grid density and extent
		float step = (gridExtent * 2.0f) / gridDensity;

		// Generate vertices for the horizontal grid lines
		for (int i = 0; i <= gridDensity; i++)
		{
			// Calculate the y-coordinate for the horizontal line
			float y = -gridExtent + i * step;

			// Create two vertices for the horizontal line
			mVertex v1 = { -gridExtent, y, 0.0f, 1.0f };
			mVertex v2 = { gridExtent, y, 0.0f, 1.0f };

			// Add the vertices to the output vector
			outVertices.push_back(v1);
			outVertices.push_back(v2);

		}
		
		// Generate vertices for the vertical grid lines
		for (int j = 0; j <= gridDensity; j++)
		{
			// Calculate the x-coordinate for the vertical line
			float x = -gridExtent + j * step;

			// Create two vertices for the vertical line
			mVertex v1 = { x, -gridExtent, 0.0f, 1.0f };
			mVertex v2 = { x, gridExtent, 0.0f, 1.0f };
			
			// Add the vertices to the output vector
			outVertices.push_back(v1);
			outVertices.push_back(v2);

		}

	}

	void InitializeVertexBuffer()
	{
		// TODO: Part 1b
		// TODO: Part 1c
		// TODO: Part 1d
		std::vector<mVertex> gridVertices;
		const int gridDensity = 25; // 25 x 25 grid
		const float gridExtent = 0.5f; // Half of NDC(-0.5 to 0.5)
		
		// Create grid vertices
		CreateGridVertices(gridVertices, gridDensity, gridExtent);

		// Create vertex buffer with the grid vertices
		CreateVertexBuffer(gridVertices.data(), static_cast<unsigned int>(gridVertices.size() * sizeof(mVertex)));
	}

	void CreateVertexBuffer(const void* data, unsigned int sizeInBytes)
	{
		GvkHelper::create_buffer(physicalDevice, device, sizeInBytes,VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&vertexHandle, &vertexData);
		GvkHelper::write_to_buffer(device, vertexData, data, sizeInBytes); // Transfer triangle data to the vertex buffer. (staging would be prefered here)
	}

	void CompileShaders()
	{
		// Intialize runtime shader compiler HLSL -> SPIRV
		shaderc_compiler_t compiler = shaderc_compiler_initialize();
		shaderc_compile_options_t options = CreateCompileOptions();

		CompileVertexShader(compiler, options);
		CompileFragmentShader(compiler, options);

		// Free runtime shader compiler resources
		shaderc_compile_options_release(options);
		shaderc_compiler_release(compiler);
	}

	shaderc_compile_options_t CreateCompileOptions()
	{
		shaderc_compile_options_t retval = shaderc_compile_options_initialize();
		shaderc_compile_options_set_source_language(retval, shaderc_source_language_hlsl);
		shaderc_compile_options_set_invert_y(retval, false);	// TODO: Part 3e
#ifndef NDEBUG
		shaderc_compile_options_set_generate_debug_info(retval);
#endif
		return retval;
	}

	void CompileVertexShader(const shaderc_compiler_t& compiler, const shaderc_compile_options_t& options)
	{
		std::string vertexShaderSource = ReadFileIntoString("../VertexShader.hlsl");
		
		shaderc_compilation_result_t result = shaderc_compile_into_spv( // compile
			compiler, vertexShaderSource.c_str(), vertexShaderSource.length(),
			shaderc_vertex_shader, "main.vert", "main", options);

		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
		{
			PrintLabeledDebugString("Vertex Shader Errors: \n", shaderc_result_get_error_message(result));
			abort(); //Vertex shader failed to compile! 
			return;
		}

		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &vertexShader);

		shaderc_result_release(result); // done
	}

	void CompileFragmentShader(const shaderc_compiler_t& compiler, const shaderc_compile_options_t& options)
	{
		std::string fragmentShaderSource = ReadFileIntoString("../FragmentShader.hlsl");

		shaderc_compilation_result_t result = shaderc_compile_into_spv( // compile
			compiler, fragmentShaderSource.c_str(), fragmentShaderSource.length(),
			shaderc_fragment_shader, "main.frag", "main", options);

		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
		{
			PrintLabeledDebugString("Fragment Shader Errors: \n", shaderc_result_get_error_message(result));
			abort(); //Fragment shader failed to compile! 
			return;
		}

		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &fragmentShader);

		shaderc_result_release(result); // done
	}

	// Create Pipeline & Layout (Thanks Tiny!)
	void InitializeGraphicsPipeline()
	{
		VkPipelineShaderStageCreateInfo stage_create_info[2] = {};	

		// Create Stage Info for Vertex Shader
		stage_create_info[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stage_create_info[0].module = vertexShader;
		stage_create_info[0].pName = "main";

		// Create Stage Info for Fragment Shader
		stage_create_info[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stage_create_info[1].module = fragmentShader;
		stage_create_info[1].pName = "main";


		VkPipelineInputAssemblyStateCreateInfo assembly_create_info = CreateVkPipelineInputAssemblyStateCreateInfo();
		VkVertexInputBindingDescription vertex_binding_description = CreateVkVertexInputBindingDescription();

		// TODO: Part 1c
		VkVertexInputAttributeDescription vertex_attribute_descriptions[1];
		vertex_attribute_descriptions[0].binding = 0;
		vertex_attribute_descriptions[0].location = 0;
		vertex_attribute_descriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		vertex_attribute_descriptions[0].offset = 0;

		VkPipelineVertexInputStateCreateInfo input_vertex_info = CreateVkPipelineVertexInputStateCreateInfo(&vertex_binding_description, 1, vertex_attribute_descriptions, 1);
		VkViewport viewport = CreateViewportFromWindowDimensions();
		VkRect2D scissor = CreateScissorFromWindowDimensions();
		VkPipelineViewportStateCreateInfo viewport_create_info = CreateVkPipelineViewportStateCreateInfo(&viewport, 1, &scissor, 1);
		VkPipelineRasterizationStateCreateInfo rasterization_create_info = CreateVkPipelineRasterizationStateCreateInfo();
		VkPipelineMultisampleStateCreateInfo multisample_create_info = CreateVkPipelineMultisampleStateCreateInfo();
		VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info = CreateVkPipelineDepthStencilStateCreateInfo();
		VkPipelineColorBlendAttachmentState color_blend_attachment_state = CreateVkPipelineColorBlendAttachmentState();
		VkPipelineColorBlendStateCreateInfo color_blend_create_info = CreateVkPipelineColorBlendStateCreateInfo(&color_blend_attachment_state, 1);

		VkDynamicState dynamic_states[2] = 
		{
			// By setting these we do not need to re-create the pipeline on Resize
			VK_DYNAMIC_STATE_VIEWPORT, 
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamic_create_info = CreateVkPipelineDynamicStateCreateInfo(dynamic_states, 2);

		CreatePipelineLayout();

		// Pipeline State... (FINALLY) 
		VkGraphicsPipelineCreateInfo pipeline_create_info = {};
		pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_create_info.stageCount = 2;
		pipeline_create_info.pStages = stage_create_info;
		pipeline_create_info.pInputAssemblyState = &assembly_create_info;
		pipeline_create_info.pVertexInputState = &input_vertex_info;
		pipeline_create_info.pViewportState = &viewport_create_info;
		pipeline_create_info.pRasterizationState = &rasterization_create_info;
		pipeline_create_info.pMultisampleState = &multisample_create_info;
		pipeline_create_info.pDepthStencilState = &depth_stencil_create_info;
		pipeline_create_info.pColorBlendState = &color_blend_create_info;
		pipeline_create_info.pDynamicState = &dynamic_create_info;
		pipeline_create_info.layout = pipelineLayout;
		pipeline_create_info.renderPass = renderPass;
		pipeline_create_info.subpass = 0;
		pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
		
		vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &pipeline);
	}

	VkPipelineShaderStageCreateInfo CreateVertexShaderStageCreateInfo()
	{
		VkPipelineShaderStageCreateInfo retval;
		retval.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		retval.stage = VK_SHADER_STAGE_VERTEX_BIT;
		retval.module = vertexShader;
		retval.pName = "main";
		return retval;
	}

	VkPipelineInputAssemblyStateCreateInfo CreateVkPipelineInputAssemblyStateCreateInfo()
	{
		VkPipelineInputAssemblyStateCreateInfo retval = {};
		retval.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		retval.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST; // TODO: Part 1b
		retval.primitiveRestartEnable = false;
		return retval;
	}

	VkVertexInputBindingDescription CreateVkVertexInputBindingDescription()
	{
		VkVertexInputBindingDescription retval = {};
		retval.binding = 0;
		retval.stride = sizeof(mVertex); 	//TODO: Part 1c
		retval.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return retval;
	}

	VkPipelineVertexInputStateCreateInfo CreateVkPipelineVertexInputStateCreateInfo(
		VkVertexInputBindingDescription* inputBindingDescriptions, unsigned int bindingCount,
		VkVertexInputAttributeDescription* vertexAttributeDescriptions, unsigned int attributeCount)
	{
		VkPipelineVertexInputStateCreateInfo retval = {};
		retval.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		retval.vertexBindingDescriptionCount = bindingCount;
		retval.pVertexBindingDescriptions = inputBindingDescriptions;
		retval.vertexAttributeDescriptionCount = attributeCount;
		retval.pVertexAttributeDescriptions = vertexAttributeDescriptions;
		return retval;
	}

	VkViewport CreateViewportFromWindowDimensions()
	{
		VkViewport retval = {};
		retval.x = 0;
		retval.y = 0;
		retval.width = static_cast<float>(windowWidth);
		retval.height = static_cast<float>(windowHeight);
		retval.minDepth = 0;
		retval.maxDepth = 1;
		return retval;
	}

	VkRect2D CreateScissorFromWindowDimensions()
	{
		VkRect2D retval = {};
		retval.offset.x = 0;
		retval.offset.y = 0;
		retval.extent.width = windowWidth;
		retval.extent.height = windowHeight;
		return retval;
	}

	VkPipelineViewportStateCreateInfo CreateVkPipelineViewportStateCreateInfo(VkViewport* viewports, unsigned int viewportCount, VkRect2D* scissors, unsigned int scissorCount)
	{
		VkPipelineViewportStateCreateInfo retval = {};
		retval.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		retval.viewportCount = viewportCount;
		retval.pViewports = viewports;
		retval.scissorCount = scissorCount;
		retval.pScissors = scissors;
		return retval;
	}

	VkPipelineRasterizationStateCreateInfo CreateVkPipelineRasterizationStateCreateInfo()
	{
		VkPipelineRasterizationStateCreateInfo retval = {};
		retval.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		retval.rasterizerDiscardEnable = VK_FALSE;
		retval.polygonMode = VK_POLYGON_MODE_FILL;
		retval.lineWidth = 1.0f;
		retval.cullMode = VK_CULL_MODE_BACK_BIT;
		retval.frontFace = VK_FRONT_FACE_CLOCKWISE;
		retval.depthClampEnable = VK_FALSE;
		retval.depthBiasEnable = VK_FALSE;
		retval.depthBiasClamp = 0.0f;
		retval.depthBiasConstantFactor = 0.0f;
		retval.depthBiasSlopeFactor = 0.0f;
		return retval;
	}

	VkPipelineMultisampleStateCreateInfo CreateVkPipelineMultisampleStateCreateInfo()
	{
		VkPipelineMultisampleStateCreateInfo retval = {};
		retval.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		retval.sampleShadingEnable = VK_FALSE;
		retval.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		retval.minSampleShading = 1.0f;
		retval.pSampleMask = VK_NULL_HANDLE;
		retval.alphaToCoverageEnable = VK_FALSE;
		retval.alphaToOneEnable = VK_FALSE;
		return retval;
	}

	VkPipelineDepthStencilStateCreateInfo CreateVkPipelineDepthStencilStateCreateInfo()
	{
		VkPipelineDepthStencilStateCreateInfo retval = {};
		retval.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		retval.depthTestEnable = VK_TRUE;
		retval.depthWriteEnable = VK_TRUE;
		retval.depthCompareOp = VK_COMPARE_OP_LESS;
		retval.depthBoundsTestEnable = VK_FALSE;
		retval.minDepthBounds = 0.0f;
		retval.maxDepthBounds = 1.0f;
		retval.stencilTestEnable = VK_FALSE;
		return retval;
	}

	VkPipelineColorBlendAttachmentState CreateVkPipelineColorBlendAttachmentState()
	{
		VkPipelineColorBlendAttachmentState retval = {};
		retval.colorWriteMask = 0xF;
		retval.blendEnable = VK_FALSE;
		retval.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
		retval.dstColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
		retval.colorBlendOp = VK_BLEND_OP_ADD;
		retval.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		retval.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
		retval.alphaBlendOp = VK_BLEND_OP_ADD;
		return retval;
	}

	VkPipelineColorBlendStateCreateInfo CreateVkPipelineColorBlendStateCreateInfo(VkPipelineColorBlendAttachmentState* attachments, unsigned int attachmentCount)
	{
		VkPipelineColorBlendStateCreateInfo retval = {};
		retval.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		retval.logicOpEnable = VK_FALSE;
		retval.logicOp = VK_LOGIC_OP_COPY;
		retval.attachmentCount = attachmentCount;
		retval.pAttachments = attachments;
		retval.blendConstants[0] = 0.0f;
		retval.blendConstants[1] = 0.0f;
		retval.blendConstants[2] = 0.0f;
		retval.blendConstants[3] = 0.0f;
		return retval;
	}

	VkPipelineDynamicStateCreateInfo CreateVkPipelineDynamicStateCreateInfo(VkDynamicState* dynamicStates, unsigned int dynamicStateCount)
	{
		VkPipelineDynamicStateCreateInfo retval = {};
		retval.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		retval.dynamicStateCount = dynamicStateCount;
		retval.pDynamicStates = dynamicStates;
		return retval;
	}

	void CreatePipelineLayout()
	{

		VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
		pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_create_info.setLayoutCount = 1; // TODO: Part 2e
		pipeline_layout_create_info.pSetLayouts = &descriptorSetLayout; // TODO: Part 2e
		pipeline_layout_create_info.pushConstantRangeCount = 0; 
		pipeline_layout_create_info.pPushConstantRanges = nullptr;

		vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &pipelineLayout);
	}

	void BindShutdownCallback()
	{
		// GVulkanSurface will inform us when to release any allocated resources
		shutdown.Create(vlk, [&]() {
			if (+shutdown.Find(GW::GRAPHICS::GVulkanSurface::Events::RELEASE_RESOURCES, true)) {
				CleanUp(); // unlike D3D we must be careful about destroy timing
			}
			});
	}


public:
	void Render()
	{
		// Calculate delta time (in a real application, this would come from a timer)
		static float lastFrameTime = 0.0f;
		float currentTime = static_cast<float>(GetTickCount64()) / 1000.0f;
		float deltaTime = currentTime - lastFrameTime;
		lastFrameTime = currentTime;

		// Update the view matrix
		UpdateViewMatrix(deltaTime);

		VkCommandBuffer commandBuffer = GetCurrentCommandBuffer();
		SetUpPipeline(commandBuffer);

		// TODO: Part 4x
		// TODO: Part 2b
		// Initialize shader variables
		SHADER_VARS shaderData;
		// Copy all world matrices to the shader data
		for (int i = 0; i < 6; i++) {
			shaderData.world[i] = worldMatrices[i];
		}
		shaderData.view = viewMatrix;
		shaderData.projection = projectionMatrix;

		// TODO: Part 2i // TODO: Part 4y
		unsigned int currentBuffer;
		vlk.GetSwapchainCurrentImage(currentBuffer);
		GvkHelper::write_to_buffer(device, uniformBuffersMemory[currentBuffer], &shaderData, sizeof(SHADER_VARS));

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
			&descriptorSets[currentBuffer], 0, nullptr);

		// TODO: Part 3g
		const int gridDensity = 25; // 25 x 25 grid
		// 2 vertices per line, 2 lines per grid cell (lines) * (directions) * (vertices per line)
		const int totalVertices = (gridDensity + 1) * 2 * 2;
		vkCmdDraw(commandBuffer, totalVertices, 6, 0, 0); // TODO: Part 1b 
	}

	// TODO: Part 4b
	// TODO: Part 4c
	// TODO: Part 4d
	// TODO: Part 4e
	// TODO: Part 4f
	// TODO: Part 4g

private:
	VkCommandBuffer GetCurrentCommandBuffer()
	{
		unsigned int currentBuffer;
		vlk.GetSwapchainCurrentImage(currentBuffer);

		VkCommandBuffer commandBuffer;
		vlk.GetCommandBuffer(currentBuffer, (void**)&commandBuffer);
		return commandBuffer;
	}

	void SetUpPipeline(VkCommandBuffer& commandBuffer)
	{
		UpdateWindowDimensions();
		SetViewport(commandBuffer);
		SetScissor(commandBuffer);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		BindVertexBuffers(commandBuffer);
	}

	void SetViewport(const VkCommandBuffer& commandBuffer)
	{
		VkViewport viewport = CreateViewportFromWindowDimensions();
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	}

	void SetScissor(const VkCommandBuffer& commandBuffer)
	{
		VkRect2D scissor = CreateScissorFromWindowDimensions();
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	void BindVertexBuffers(VkCommandBuffer& commandBuffer)
	{
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexHandle, offsets);
	}


	//Cleanup callback function (passed to VKSurface, will be called when the pipeline shuts down)
	void CleanUp()
	{
		// wait till everything has completed
		vkDeviceWaitIdle(device);

		// Release allocated buffers, shaders & pipeline
		vkDestroyBuffer(device, vertexHandle, nullptr);
		vkFreeMemory(device, vertexData, nullptr);

		// TODO: Part 2d
		for (size_t i = 0; i < uniformBuffers.size(); ++i)
		{
			vkDestroyBuffer(device, uniformBuffers[i], nullptr);
			vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
		}

		// TODO: Part 2f
		vkDestroyDescriptorPool(device, descriptorPool, nullptr);

		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
		vkDestroyShaderModule(device, vertexShader, nullptr);
		vkDestroyShaderModule(device, fragmentShader, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyPipeline(device, pipeline, nullptr);
	}
};
