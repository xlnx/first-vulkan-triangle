#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <optional>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

using namespace std;
using namespace glm;

// using namespace vk;

constexpr auto MAX_FRAMES_IN_FLIGHT = 2;

vector<const char *> device_extensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct QueueFamilyIndices
{
	optional<uint32_t> graphics_family;
	optional<uint32_t> present_family;
};

struct SwapchainSupportDetails
{
	vk::SurfaceCapabilitiesKHR capabilities;
	vector<vk::SurfaceFormatKHR> formats;
	vector<vk::PresentModeKHR> present_modes;
};

struct Application
{
	Application()
	{
		initWindow();
		initVulkan();
	}
	~Application()
	{
		glfwDestroyWindow( window );
		glfwTerminate();
		inst.destroy();
	}

	void run()
	{
		while ( !glfwWindowShouldClose( window ) ) {
			glfwPollEvents();
			drawFrame();
		}

		device.waitIdle();
	}

private:
	void initWindow()
	{
		glfwInit();
		glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
		window = glfwCreateWindow( 800, 600, "main", nullptr, nullptr );
	}
	void initVulkan()
	{
		createInstance();
		setupDebugMessenger();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapchain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandPool();
		createCommandBuffers();
		createSyncObjects();
	}

private:
	void createInstance()
	{
		vk::ApplicationInfo appInfo = {};
		appInfo.apiVersion = VK_API_VERSION_1_0;
		appInfo.pApplicationName = "...";
		appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
		appInfo.pEngineName = "...";
		appInfo.engineVersion = VK_MAKE_VERSION( 1, 0, 0 );

		vk::InstanceCreateInfo createInfo = {};
		createInfo.pApplicationInfo = &appInfo;

		auto extensions = getRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>( extensions.size() );
		createInfo.ppEnabledExtensionNames = extensions.data();

		vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
		if ( false ) {
			//
		}

		if ( vk::createInstance( &createInfo, nullptr, &inst ) != vk::Result::eSuccess ) {
			throw std::runtime_error( "unable to create instance" );
		}
	}

	void setupDebugMessenger()
	{
		if ( not false ) return;
	}

	void createSurface()
	{
		VkSurfaceKHR surface;
		if ( glfwCreateWindowSurface( inst, window, nullptr, &surface ) != VK_SUCCESS ) {
			throw std::runtime_error( "unable to create window surface" );
		}
		this->surface = surface;
	}

	void pickPhysicalDevice()
	{
		auto devices = inst.enumeratePhysicalDevices();
		if ( not devices.size() ) {
			throw std::runtime_error( "failed to find GPUs with Vulkan support" );
		}
		physical_device = devices.front();
	}

	auto findQueueFamilies( const vk::PhysicalDevice &device )
	{
		QueueFamilyIndices indices;
		auto queue_families = device.getQueueFamilyProperties();

		uint32_t idx = 0;
		for ( auto &queue_family : queue_families ) {
			if ( queue_family.queueCount > 0 && queue_family.queueFlags & vk::QueueFlagBits::eGraphics ) {
				indices.graphics_family = idx;
			}
			if ( queue_family.queueCount > 0 && device.getSurfaceSupportKHR( idx, surface ) ) {
				indices.present_family = idx;
			}
			if ( indices.graphics_family.has_value() && indices.present_family.has_value() ) {
				break;
			}
			++idx;
		}
		return indices;
	}

	void createLogicalDevice()
	{
		auto indices = findQueueFamilies( physical_device );

		vector<vk::DeviceQueueCreateInfo> queue_create_infos;
		set<uint32_t> unique_queue_families = {
			indices.graphics_family.value(),
			indices.present_family.value()
		};

		float queue_priority = 1.f;
		for ( auto queue_family : unique_queue_families ) {
			auto queue_create_info =
			  vk::DeviceQueueCreateInfo()
				.setQueueFamilyIndex( queue_family )
				.setQueueCount( 1 )
				.setPQueuePriorities( &queue_priority );
			queue_create_infos.emplace_back( queue_create_info );
		}

		auto device_features = vk::PhysicalDeviceFeatures();

		auto create_info =
		  vk::DeviceCreateInfo()
			.setQueueCreateInfoCount( queue_create_infos.size() )
			.setPQueueCreateInfos( queue_create_infos.data() )
			.setPEnabledFeatures( &device_features )
			.setEnabledExtensionCount( device_extensions.size() )
			.setPpEnabledExtensionNames( device_extensions.data() );

		if ( false ) {
			// ...
		}

		if ( physical_device.createDevice( &create_info, nullptr, &device ) != vk::Result::eSuccess ) {
			throw std::runtime_error( "failed to create logical device" );
		}

		graphics_queue = device.getQueue( indices.graphics_family.value(), 0 );
		present_queue = device.getQueue( indices.present_family.value(), 0 );
	}

	void createSwapchain()
	{
		auto swap_chain_support = querySwapchainSupport( physical_device );

		auto surface_format = chooseSwapSurfaceFormat( swap_chain_support.formats );
		auto present_mode = chooseSwapPresentMode( swap_chain_support.present_modes );
		auto extent = chooseSwapExtent( swap_chain_support.capabilities );

		uint32_t image_cnt = swap_chain_support.capabilities.minImageCount + 1;
		if ( swap_chain_support.capabilities.maxImageCount > 0 && image_cnt > swap_chain_support.capabilities.maxImageCount ) {
			image_cnt = swap_chain_support.capabilities.maxImageCount;
		}

		auto create_info =
		  vk::SwapchainCreateInfoKHR()
			.setSurface( surface )
			.setMinImageCount( image_cnt )
			.setImageFormat( surface_format.format )
			.setImageColorSpace( surface_format.colorSpace )
			.setImageExtent( extent )
			.setImageArrayLayers( 1 )
			.setImageUsage( vk::ImageUsageFlagBits::eColorAttachment );

		auto indices = findQueueFamilies( physical_device );
		uint32_t queue_family_indices[] = {
			indices.graphics_family.value(),
			indices.present_family.value()
		};

		if ( indices.graphics_family != indices.present_family ) {
			create_info.setImageSharingMode( vk::SharingMode::eConcurrent )
			  .setQueueFamilyIndexCount( 2 )
			  .setPQueueFamilyIndices( queue_family_indices );
		} else {
			create_info.setImageSharingMode( vk::SharingMode::eExclusive );
		}

		create_info.setPreTransform( swap_chain_support.capabilities.currentTransform )
		  .setCompositeAlpha( vk::CompositeAlphaFlagBitsKHR::eOpaque )
		  .setPresentMode( present_mode )
		  .setClipped( true );

		if ( device.createSwapchainKHR( &create_info, nullptr, &swap_chain ) != vk::Result::eSuccess ) {
			throw std::runtime_error( "failed to create swap chain" );
		}

		swap_chain_images = device.getSwapchainImagesKHR( swap_chain );

		swap_chain_image_format = surface_format.format;
		swap_chain_extent = extent;
	}

	void createImageViews()
	{
		swap_chain_image_views.resize( swap_chain_images.size() );
		for ( size_t i = 0; i < swap_chain_images.size(); ++i ) {
			auto create_info =
			  vk::ImageViewCreateInfo()
				.setImage( swap_chain_images[ i ] )
				.setViewType( vk::ImageViewType::e2D )
				.setFormat( swap_chain_image_format )
				.setComponents( { vk::ComponentSwizzle::eIdentity,
								  vk::ComponentSwizzle::eIdentity,
								  vk::ComponentSwizzle::eIdentity,
								  vk::ComponentSwizzle::eIdentity } );
			create_info.subresourceRange
			  .setAspectMask( vk::ImageAspectFlagBits::eColor )
			  .setBaseMipLevel( 0 )
			  .setLevelCount( 1 )
			  .setBaseArrayLayer( 0 )
			  .setLayerCount( 1 );

			if ( device.createImageView( &create_info, nullptr, &swap_chain_image_views[ i ] ) != vk::Result::eSuccess ) {
				throw std::runtime_error( "failed to create image views" );
			}
		}
	}

	void createRenderPass()
	{
		auto color_attachment =
		  vk::AttachmentDescription()
			.setFormat( swap_chain_image_format )
			.setSamples( vk::SampleCountFlagBits::e1 )
			.setLoadOp( vk::AttachmentLoadOp::eClear )
			.setStoreOp( vk::AttachmentStoreOp::eStore )
			.setStencilLoadOp( vk::AttachmentLoadOp::eDontCare )
			.setStencilStoreOp( vk::AttachmentStoreOp::eDontCare )
			.setInitialLayout( vk::ImageLayout::eUndefined )
			.setFinalLayout( vk::ImageLayout::ePresentSrcKHR );

		auto color_attachment_ref =
		  vk::AttachmentReference()
			.setAttachment( 0 )
			.setLayout( vk::ImageLayout::eColorAttachmentOptimal );

		auto subpass =
		  vk::SubpassDescription()
			.setPipelineBindPoint( vk::PipelineBindPoint::eGraphics )
			.setColorAttachmentCount( 1 )
			.setPColorAttachments( &color_attachment_ref );

		auto dependency =
		  vk::SubpassDependency()
			.setSrcSubpass( VK_SUBPASS_EXTERNAL )
			.setDstSubpass( 0 )
			.setSrcStageMask( vk::PipelineStageFlagBits::eColorAttachmentOutput )
			.setDstStageMask( vk::PipelineStageFlagBits::eColorAttachmentOutput )
			.setDstAccessMask( vk::AccessFlagBits::eColorAttachmentRead |
							   vk::AccessFlagBits::eColorAttachmentWrite );

		auto render_pass_info =
		  vk::RenderPassCreateInfo()
			.setAttachmentCount( 1 )
			.setPAttachments( &color_attachment )
			.setSubpassCount( 1 )
			.setPSubpasses( &subpass )
			.setDependencyCount( 1 )
			.setPDependencies( &dependency );

		if ( device.createRenderPass( &render_pass_info, nullptr, &render_pass ) != vk::Result::eSuccess ) {
			throw std::runtime_error( "failed to create render pass" );
		}
	}

	void createGraphicsPipeline()
	{
		auto vert_shader = readFile( "vs.spv" );
		auto frag_shader = readFile( "fs.spv" );

		vk::ShaderModule vert_shader_module = createShaderModule( vert_shader );
		vk::ShaderModule frag_shader_module = createShaderModule( frag_shader );

		auto vert_shader_stage_info =
		  vk::PipelineShaderStageCreateInfo()
			.setStage( vk::ShaderStageFlagBits::eVertex )
			.setModule( vert_shader_module )
			.setPName( "main" );

		auto frag_shader_stage_info =
		  vk::PipelineShaderStageCreateInfo()
			.setStage( vk::ShaderStageFlagBits::eFragment )
			.setModule( frag_shader_module )
			.setPName( "main" );

		vk::PipelineShaderStageCreateInfo shader_stages[] = {
			vert_shader_stage_info,
			frag_shader_stage_info
		};

		auto vertex_input_info =
		  vk::PipelineVertexInputStateCreateInfo()
			.setVertexBindingDescriptionCount( 0 )
			.setVertexAttributeDescriptionCount( 0 );

		auto input_asm =
		  vk::PipelineInputAssemblyStateCreateInfo()
			.setTopology( vk::PrimitiveTopology::eTriangleList )
			.setPrimitiveRestartEnable( false );

		auto viewport =
		  vk::Viewport()
			.setX( 0.f )
			.setY( 0.f )
			.setWidth( swap_chain_extent.width )
			.setHeight( swap_chain_extent.height )
			.setMinDepth( 0.f )
			.setMaxDepth( 1.f );

		auto scissor =
		  vk::Rect2D()
			.setOffset( vk::Offset2D{ 0, 0 } )
			.setExtent( swap_chain_extent );

		auto viewport_state =
		  vk::PipelineViewportStateCreateInfo()
			.setViewportCount( 1 )
			.setPViewports( &viewport )
			.setScissorCount( 1 )
			.setPScissors( &scissor );

		auto rasterizer =
		  vk::PipelineRasterizationStateCreateInfo()
			.setDepthClampEnable( false )
			.setRasterizerDiscardEnable( false )
			.setPolygonMode( vk::PolygonMode::eFill )
			.setLineWidth( 1.f )
			.setCullMode( vk::CullModeFlagBits::eBack )
			.setFrontFace( vk::FrontFace::eClockwise )
			.setDepthClampEnable( false );

		auto multisampling =
		  vk::PipelineMultisampleStateCreateInfo()
			.setSampleShadingEnable( false )
			.setRasterizationSamples( vk::SampleCountFlagBits::e1 );

		auto color_blend_attatchment =
		  vk::PipelineColorBlendAttachmentState()
			.setColorWriteMask( vk::ColorComponentFlagBits::eR |
								vk::ColorComponentFlagBits::eG |
								vk::ColorComponentFlagBits::eB |
								vk::ColorComponentFlagBits::eA )
			.setBlendEnable( false );

		auto color_blending =
		  vk::PipelineColorBlendStateCreateInfo()
			.setLogicOpEnable( false )
			.setLogicOp( vk::LogicOp::eCopy )
			.setAttachmentCount( 1 )
			.setPAttachments( &color_blend_attatchment )
			.setBlendConstants( { 0.f, 0.f, 0.f, 0.f } );

		auto pipeline_layout_info =
		  vk::PipelineLayoutCreateInfo()
			.setSetLayoutCount( 0 )
			.setPushConstantRangeCount( 0 );

		if ( device.createPipelineLayout( &pipeline_layout_info, nullptr, &pipeline_layout ) != vk::Result::eSuccess ) {
			throw std::runtime_error( "failed to create pipeline layout" );
		}

		auto pipeline_info =
		  vk::GraphicsPipelineCreateInfo()
			.setStageCount( 2 )
			.setPStages( shader_stages )
			.setPVertexInputState( &vertex_input_info )
			.setPInputAssemblyState( &input_asm )
			.setPViewportState( &viewport_state )
			.setPRasterizationState( &rasterizer )
			.setPMultisampleState( &multisampling )
			.setPColorBlendState( &color_blending )
			.setLayout( pipeline_layout )
			.setRenderPass( render_pass )
			.setSubpass( 0 );
		// pipeline_info.basePipelineHandle =

		if ( device.createGraphicsPipelines( vk::PipelineCache{}, 1, &pipeline_info, nullptr, &graphics_pipeline ) != vk::Result::eSuccess ) {
			throw std::runtime_error( "failed to create graphics pipeline" );
		}

		device.destroy( vert_shader_module );
		device.destroy( frag_shader_module );
	}

	void createFramebuffers()
	{
		swap_chain_frame_buffers.resize( swap_chain_image_views.size() );
		for ( size_t i = 0; i < swap_chain_frame_buffers.size(); ++i ) {
			vk::ImageView attachments[] = { swap_chain_image_views[ i ] };

			auto frame_buffer_info =
			  vk::FramebufferCreateInfo()
				.setRenderPass( render_pass )
				.setAttachmentCount( 1 )
				.setPAttachments( attachments )
				.setWidth( swap_chain_extent.width )
				.setHeight( swap_chain_extent.height )
				.setLayers( 1 );

			if ( device.createFramebuffer( &frame_buffer_info, nullptr,
										   &swap_chain_frame_buffers[ i ] ) != vk::Result::eSuccess ) {
				throw std::runtime_error( "failed to create framebuffer" );
			}
		}
	}

	void createCommandPool()
	{
		auto indices = findQueueFamilies( physical_device );
		auto pool_info =
		  vk::CommandPoolCreateInfo()
			.setQueueFamilyIndex( indices.graphics_family.value() );

		if ( device.createCommandPool( &pool_info, nullptr, &command_pool ) != vk::Result::eSuccess ) {
			throw std::runtime_error( "failed to create command pool" );
		}
	}

	void createCommandBuffers()
	{
		command_buffers.resize( swap_chain_frame_buffers.size() );

		auto alloc_info =
		  vk::CommandBufferAllocateInfo()
			.setCommandPool( command_pool )
			.setLevel( vk::CommandBufferLevel::ePrimary )
			.setCommandBufferCount( command_buffers.size() );

		if ( device.allocateCommandBuffers( &alloc_info, command_buffers.data() ) != vk::Result::eSuccess ) {
			throw std::runtime_error( "failed to allocate command buffer" );
		}

		for ( size_t i = 0; i < command_buffers.size(); ++i ) {
			auto begin_info = vk::CommandBufferBeginInfo();

			if ( command_buffers[ i ].begin( &begin_info ) != vk::Result::eSuccess ) {
				throw std::runtime_error( "failed to begin recording command buffer" );
			}

			auto render_pass_info =
			  vk::RenderPassBeginInfo()
				.setRenderPass( render_pass )
				.setFramebuffer( swap_chain_frame_buffers[ i ] );
			render_pass_info.renderArea
			  .setOffset( 0 )
			  .setExtent( swap_chain_extent );

			vk::ClearValue clear_color =
			  vk::ClearColorValue()
				.setFloat32( { 0.f, 0.f, 0.f, 1.f } );
			render_pass_info.setClearValueCount( 1 )
			  .setPClearValues( &clear_color );

			command_buffers[ i ].beginRenderPass( &render_pass_info, vk::SubpassContents::eInline );
			command_buffers[ i ].bindPipeline( vk::PipelineBindPoint::eGraphics, graphics_pipeline );
			command_buffers[ i ].draw( 3, 1, 0, 0 );
			command_buffers[ i ].endRenderPass();

			if ( vkEndCommandBuffer( command_buffers[ i ] ) != VK_SUCCESS ) {
				// if ( command_buffers[ i ].end() != vk::Result::eSuccess ) {
				throw std::runtime_error( "failed to record command buffer" );
			}
		}
	}

	void createSyncObjects()
	{
		image_avail_semaphores.resize( MAX_FRAMES_IN_FLIGHT );
		render_finish_semaphores.resize( MAX_FRAMES_IN_FLIGHT );
		in_flight_fences.resize( MAX_FRAMES_IN_FLIGHT );

		auto semaphore_info = vk::SemaphoreCreateInfo();

		auto fence_info =
		  vk::FenceCreateInfo()
			.setFlags( vk::FenceCreateFlagBits::eSignaled );

		for ( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i ) {
			if ( device.createSemaphore( &semaphore_info, nullptr,
										 &image_avail_semaphores[ i ] ) != vk::Result::eSuccess ||
				 device.createSemaphore( &semaphore_info, nullptr,
										 &render_finish_semaphores[ i ] ) != vk::Result::eSuccess ||
				 device.createFence( &fence_info, nullptr, &in_flight_fences[ i ] ) != vk::Result::eSuccess ) {
				throw std::runtime_error( "failed to create synchronization objects for a frame" );
			}
		}
	}

	vk::ShaderModule createShaderModule( const vector<char> &code )
	{
		auto create_info =
		  vk::ShaderModuleCreateInfo()
			.setCodeSize( code.size() )
			.setPCode( reinterpret_cast<const uint32_t *>( code.data() ) );

		vk::ShaderModule shader_module;
		if ( device.createShaderModule( &create_info, nullptr, &shader_module ) != vk::Result::eSuccess ) {
			throw std::runtime_error( "failed to create shader module" );
		}

		return shader_module;
	}

	SwapchainSupportDetails querySwapchainSupport( const vk::PhysicalDevice &device )
	{
		SwapchainSupportDetails details;
		details.capabilities = device.getSurfaceCapabilitiesKHR( surface );
		details.formats = device.getSurfaceFormatsKHR( surface );
		details.present_modes = device.getSurfacePresentModesKHR( surface );
		return details;
	}

	vk::SurfaceFormatKHR chooseSwapSurfaceFormat( const vector<vk::SurfaceFormatKHR> &avail )
	{
		for ( auto &avail_format : avail ) {
			if ( avail_format.format == vk::Format::eB8G8R8A8Unorm &&
				 avail_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear ) {
				return avail_format;
			}
		}
		return avail[ 0 ];
	}

	vk::PresentModeKHR chooseSwapPresentMode( const vector<vk::PresentModeKHR> &avail )
	{
		for ( auto &avail_present_mode : avail ) {
			if ( avail_present_mode == vk::PresentModeKHR::eMailbox ) {
				return avail_present_mode;
			}
		}
		return vk::PresentModeKHR::eFifo;
	}

	vk::Extent2D chooseSwapExtent( const vk::SurfaceCapabilitiesKHR &capabilities )
	{
		if ( capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max() ) {
			return capabilities.currentExtent;
		} else {
			throw std::runtime_error( "not implemented" );
			// vk::Extent2D actual_extent = {width, height}
		}
	}

	vector<const char *> getRequiredExtensions()
	{
		uint32_t glfw_ext_cnt = 0;
		const char **glfw_exts;
		glfw_exts = glfwGetRequiredInstanceExtensions( &glfw_ext_cnt );
		vector<const char *> exts( glfw_exts, glfw_exts + glfw_ext_cnt );
		cout << "required extensions: " << exts.size() << endl;
		for ( auto &ext : exts ) {
			cout << ext << endl;
		}
		return exts;
	}

	void drawFrame()
	{
		device.waitForFences( 1, &in_flight_fences[ current_frame ], true,
							  std::numeric_limits<uint64_t>::max() );
		device.resetFences( 1, &in_flight_fences[ current_frame ] );

		uint32_t image_index;
		device.acquireNextImageKHR( swap_chain, std::numeric_limits<uint64_t>::max(),
									image_avail_semaphores[ current_frame ], vk::Fence{}, &image_index );

		vk::Semaphore wait_semaphores[] = { image_avail_semaphores[ current_frame ] };
		vk::PipelineStageFlags wait_stages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
		vk::Semaphore signal_semaphores[] = { render_finish_semaphores[ current_frame ] };

		auto submit_info =
		  vk::SubmitInfo()
			.setWaitSemaphoreCount( 1 )
			.setPWaitSemaphores( wait_semaphores )
			.setPWaitDstStageMask( wait_stages )
			.setCommandBufferCount( 1 )
			.setPCommandBuffers( &command_buffers[ image_index ] )
			.setSignalSemaphoreCount( 1 )
			.setPSignalSemaphores( signal_semaphores );

		if ( graphics_queue.submit( 1, &submit_info, in_flight_fences[ current_frame ] ) != vk::Result::eSuccess ) {
			throw std::runtime_error( "failed to submit draw command buffer" );
		}

		vk::SwapchainKHR swapchains[] = { swap_chain };

		auto present_info =
		  vk::PresentInfoKHR()
			.setWaitSemaphoreCount( 1 )
			.setPWaitSemaphores( signal_semaphores )
			.setSwapchainCount( 1 )
			.setPSwapchains( swapchains )
			.setPImageIndices( &image_index );

		present_queue.presentKHR( &present_info );

		current_frame = ( current_frame + 1 ) % MAX_FRAMES_IN_FLIGHT;
	}

	static vector<char> readFile( const string &file_name )
	{
		ifstream is( file_name, ios::ate | ios::binary );
		if ( !is.is_open() ) {
			throw std::runtime_error( "failed to open file" );
		}
		vector<char> buffer( is.tellg() );
		is.seekg( 0 );
		is.read( buffer.data(), buffer.size() );
		return buffer;
	}

private:
	GLFWwindow *window;
	vk::Instance inst;
	vk::SurfaceKHR surface;
	vk::PhysicalDevice physical_device;
	vk::Device device;
	vk::Queue graphics_queue;
	vk::Queue present_queue;
	vk::SwapchainKHR swap_chain;
	vector<vk::Image> swap_chain_images;
	vk::Format swap_chain_image_format;
	vk::Extent2D swap_chain_extent;
	vector<vk::ImageView> swap_chain_image_views;
	vk::RenderPass render_pass;
	vk::PipelineLayout pipeline_layout;
	vk::Pipeline graphics_pipeline;
	vector<vk::Framebuffer> swap_chain_frame_buffers;
	vk::CommandPool command_pool;
	vector<vk::CommandBuffer> command_buffers;
	vector<vk::Semaphore> image_avail_semaphores;
	vector<vk::Semaphore> render_finish_semaphores;
	vector<vk::Fence> in_flight_fences;

	size_t current_frame = 0;
};

int main()
{
	Application app;
	app.run();
}
