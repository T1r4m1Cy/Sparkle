#include <vulkan/vulkan.hpp>

#include <iostream>

#include "VulkanContext.h"
#include "VulkanUtils.h"

#include "VulkanPipeline.h"

int vulkan_init_graphics_pipeline(VulkanContext& ctx)
{
    auto vertShaderCode = read_file("shaders/geometry_vert.spv");
    auto fragShaderCode = read_file("shaders/geometry_frag.spv");

    vk::ShaderModule vertShaderModule = create_shader_module(ctx, vertShaderCode);
    vk::ShaderModule fragShaderModule = create_shader_module(ctx, fragShaderCode);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo = vk::PipelineShaderStageCreateInfo()
        .setStage(vk::ShaderStageFlagBits::eVertex)
        .setModule(vertShaderModule)
        .setPName("main");

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo = vk::PipelineShaderStageCreateInfo()
        .setStage(vk::ShaderStageFlagBits::eFragment)
        .setModule(fragShaderModule)
        .setPName("main");

    vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    std::vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo dynamicState = vk::PipelineDynamicStateCreateInfo()
        .setDynamicStateCount(static_cast<uint32_t>(dynamicStates.size()))
        .setPDynamicStates(dynamicStates.data());

    auto bindingDescription = Vertex::get_binding_description();
    auto attributeDescriptions = Vertex::get_attribute_descriptions();

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo = vk::PipelineVertexInputStateCreateInfo()
        .setVertexBindingDescriptionCount(1)
        .setPVertexBindingDescriptions(&bindingDescription)
        .setVertexAttributeDescriptionCount(static_cast<uint32_t>(attributeDescriptions.size()))
        .setPVertexAttributeDescriptions(attributeDescriptions.data());

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly = vk::PipelineInputAssemblyStateCreateInfo()
        .setTopology(vk::PrimitiveTopology::eTriangleList)
        .setPrimitiveRestartEnable(vk::False);

    vk::Viewport viewport = vk::Viewport()
        .setX(0.0f)
        .setY(0.0f)
        .setWidth((float)ctx.swapchain.swapchainExtent.width)
        .setHeight((float)ctx.swapchain.swapchainExtent.height)
        .setMinDepth(0.0f)
        .setMaxDepth(1.0f);

    vk::Rect2D scissor = vk::Rect2D()
        .setOffset({ 0,0 })
        .setExtent(ctx.swapchain.swapchainExtent);

    vk::PipelineViewportStateCreateInfo viewportState = vk::PipelineViewportStateCreateInfo()
        .setViewportCount(1)
        .setPViewports(&viewport)
        .setScissorCount(1)
        .setPScissors(&scissor);

    vk::PipelineRasterizationStateCreateInfo rasterizer = vk::PipelineRasterizationStateCreateInfo()
        .setDepthClampEnable(false)
        .setRasterizerDiscardEnable(false)
        .setPolygonMode(vk::PolygonMode::eFill)
        .setLineWidth(1.0f)
        .setDepthBiasEnable(false)
        .setDepthBiasConstantFactor(0.0f)
        .setDepthBiasClamp(0.0f)
        .setDepthBiasSlopeFactor(0.0f)
        .setCullMode(vk::CullModeFlagBits::eNone)
        .setFrontFace(vk::FrontFace::eCounterClockwise);

    vk::PipelineMultisampleStateCreateInfo multisampling = vk::PipelineMultisampleStateCreateInfo()
        .setSampleShadingEnable(false)
        .setRasterizationSamples(vk::SampleCountFlagBits::e1)
        .setMinSampleShading(1.0f)
        .setPSampleMask(nullptr)
        .setAlphaToCoverageEnable(false)
        .setAlphaToOneEnable(false);

    vk::PipelineDepthStencilStateCreateInfo depthStencil = vk::PipelineDepthStencilStateCreateInfo()
        .setDepthTestEnable(true)
        .setDepthWriteEnable(true)
        .setDepthCompareOp(vk::CompareOp::eLess)
        .setDepthBoundsTestEnable(false)
        .setMinDepthBounds(0.0f)
        .setMaxDepthBounds(1.0f)
        .setStencilTestEnable(false)
        .setFront({})
        .setBack({});

    vk::PipelineColorBlendAttachmentState positionColorBlendAttachment = vk::PipelineColorBlendAttachmentState()
        .setColorWriteMask(
            vk::ColorComponentFlagBits::eR |
            vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA
        )
        .setBlendEnable(false)
        .setSrcColorBlendFactor(vk::BlendFactor::eOne)
        .setDstColorBlendFactor(vk::BlendFactor::eZero)
        .setColorBlendOp(vk::BlendOp::eAdd)
        .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
        .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
        .setAlphaBlendOp(vk::BlendOp::eAdd);

    vk::PipelineColorBlendAttachmentState normalColorBlendAttachment = vk::PipelineColorBlendAttachmentState()
        .setColorWriteMask(
            vk::ColorComponentFlagBits::eR |
            vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA
        )
        .setBlendEnable(false)
        .setSrcColorBlendFactor(vk::BlendFactor::eOne)
        .setDstColorBlendFactor(vk::BlendFactor::eZero)
        .setColorBlendOp(vk::BlendOp::eAdd)
        .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
        .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
        .setAlphaBlendOp(vk::BlendOp::eAdd);

    vk::PipelineColorBlendAttachmentState albedoColorBlendAttachment = vk::PipelineColorBlendAttachmentState()
        .setColorWriteMask(
            vk::ColorComponentFlagBits::eR |
            vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA
        )
        .setBlendEnable(false)
        .setSrcColorBlendFactor(vk::BlendFactor::eOne)
        .setDstColorBlendFactor(vk::BlendFactor::eZero)
        .setColorBlendOp(vk::BlendOp::eAdd)
        .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
        .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
        .setAlphaBlendOp(vk::BlendOp::eAdd);

    std::array<vk::PipelineColorBlendAttachmentState, 3> colorBlendAttachments = {
        positionColorBlendAttachment,
        normalColorBlendAttachment,
        albedoColorBlendAttachment
    };

    vk::PipelineColorBlendStateCreateInfo colorBlending = vk::PipelineColorBlendStateCreateInfo()
        .setLogicOpEnable(false)
        .setLogicOp(vk::LogicOp::eCopy)
        .setAttachmentCount(static_cast<uint32_t>(colorBlendAttachments.size()))
        .setPAttachments(colorBlendAttachments.data())
        .setBlendConstants({ 0.0f, 0.0f, 0.0f, 0.0f });

    vk::PushConstantRange pushConstantRange = vk::PushConstantRange()
        .setStageFlags(vk::ShaderStageFlagBits::eVertex)
        .setOffset(0)
        .setSize(sizeof(glm::mat4));

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo = vk::PipelineLayoutCreateInfo()
        .setSetLayoutCount(1)
        .setPSetLayouts(&ctx.pipeline.descriptorSetLayout)
        .setPushConstantRangeCount(1)
        .setPPushConstantRanges(&pushConstantRange);

    ctx.pipeline.pipelineLayout = ctx.device.createPipelineLayout(pipelineLayoutInfo);

    vk::GraphicsPipelineCreateInfo pipelineInfo = vk::GraphicsPipelineCreateInfo()
        .setStageCount(2)
        .setPStages(shaderStages)
        .setPVertexInputState(&vertexInputInfo)
        .setPInputAssemblyState(&inputAssembly)
        .setPViewportState(&viewportState)
        .setPRasterizationState(&rasterizer)
        .setPMultisampleState(&multisampling)
        .setPDepthStencilState(&depthStencil)
        .setPColorBlendState(&colorBlending)
        .setPDynamicState(&dynamicState)
        .setLayout(ctx.pipeline.pipelineLayout)
        .setRenderPass(ctx.pipeline.renderPass)
        .setSubpass(0)
        .setBasePipelineHandle(nullptr)
        .setBasePipelineIndex(-1);

    ctx.pipeline.pipeline = ctx.device.createGraphicsPipeline(nullptr, pipelineInfo).value;

    ctx.device.destroyShaderModule(fragShaderModule);
    ctx.device.destroyShaderModule(vertShaderModule);

    return 0;
}

int vulkan_init_lighting_pipeline(VulkanContext& ctx)
{
    auto vertShaderCode = read_file("shaders/lighting_vert.spv");
    auto fragShaderCode = read_file("shaders/lighting_frag.spv");

    vk::ShaderModule vertShaderModule = create_shader_module(ctx, vertShaderCode);
    vk::ShaderModule fragShaderModule = create_shader_module(ctx, fragShaderCode);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo = vk::PipelineShaderStageCreateInfo()
        .setStage(vk::ShaderStageFlagBits::eVertex)
        .setModule(vertShaderModule)
        .setPName("main");

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo = vk::PipelineShaderStageCreateInfo()
        .setStage(vk::ShaderStageFlagBits::eFragment)
        .setModule(fragShaderModule)
        .setPName("main");

    vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    std::vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo dynamicState = vk::PipelineDynamicStateCreateInfo()
        .setDynamicStateCount(static_cast<uint32_t>(dynamicStates.size()))
        .setPDynamicStates(dynamicStates.data());

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo = vk::PipelineVertexInputStateCreateInfo()
        .setVertexBindingDescriptionCount(0)
        .setVertexAttributeDescriptionCount(0);

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly = vk::PipelineInputAssemblyStateCreateInfo()
        .setTopology(vk::PrimitiveTopology::eTriangleList)
        .setPrimitiveRestartEnable(vk::False);

    vk::Viewport viewport = vk::Viewport()
        .setX(0.0f)
        .setY(0.0f)
        .setWidth((float)ctx.swapchain.swapchainExtent.width)
        .setHeight((float)ctx.swapchain.swapchainExtent.height)
        .setMinDepth(0.0f)
        .setMaxDepth(1.0f);

    vk::Rect2D scissor = vk::Rect2D()
        .setOffset({ 0,0 })
        .setExtent(ctx.swapchain.swapchainExtent);

    vk::PipelineViewportStateCreateInfo viewportState = vk::PipelineViewportStateCreateInfo()
        .setViewportCount(1)
        .setPViewports(&viewport)
        .setScissorCount(1)
        .setPScissors(&scissor);

    vk::PipelineRasterizationStateCreateInfo rasterizer = vk::PipelineRasterizationStateCreateInfo()
        .setDepthClampEnable(false)
        .setRasterizerDiscardEnable(false)
        .setPolygonMode(vk::PolygonMode::eFill)
        .setLineWidth(1.0f)
        .setDepthBiasEnable(false)
        .setDepthBiasConstantFactor(0.0f)
        .setDepthBiasClamp(0.0f)
        .setDepthBiasSlopeFactor(0.0f)
        .setCullMode(vk::CullModeFlagBits::eNone)
        .setFrontFace(vk::FrontFace::eCounterClockwise);

    vk::PipelineMultisampleStateCreateInfo multisampling = vk::PipelineMultisampleStateCreateInfo()
        .setSampleShadingEnable(false)
        .setRasterizationSamples(vk::SampleCountFlagBits::e1)
        .setMinSampleShading(1.0f)
        .setPSampleMask(nullptr)
        .setAlphaToCoverageEnable(false)
        .setAlphaToOneEnable(false);

    vk::PipelineDepthStencilStateCreateInfo depthStencil = vk::PipelineDepthStencilStateCreateInfo()
        .setDepthTestEnable(false)
        .setDepthWriteEnable(false);

    vk::PipelineColorBlendAttachmentState colorBlendAttachment = vk::PipelineColorBlendAttachmentState()
        .setColorWriteMask(
            vk::ColorComponentFlagBits::eR |
            vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA
        )
        .setBlendEnable(false)
        .setSrcColorBlendFactor(vk::BlendFactor::eOne)
        .setDstColorBlendFactor(vk::BlendFactor::eZero)
        .setColorBlendOp(vk::BlendOp::eAdd)
        .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
        .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
        .setAlphaBlendOp(vk::BlendOp::eAdd);

    std::array<vk::PipelineColorBlendAttachmentState, 1> colorBlendAttachments = {
        colorBlendAttachment
    };

    vk::PipelineColorBlendStateCreateInfo colorBlending = vk::PipelineColorBlendStateCreateInfo()
        .setLogicOpEnable(false)
        .setLogicOp(vk::LogicOp::eCopy)
        .setAttachmentCount(static_cast<uint32_t>(colorBlendAttachments.size()))
        .setPAttachments(colorBlendAttachments.data())
        .setBlendConstants({ 0.0f, 0.0f, 0.0f, 0.0f });

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo = vk::PipelineLayoutCreateInfo()
        .setSetLayoutCount(1)
        .setPSetLayouts(&ctx.pipeline.lightingDescriptorSetLayout);

    ctx.pipeline.lightingPipelineLayout = ctx.device.createPipelineLayout(pipelineLayoutInfo);

    vk::GraphicsPipelineCreateInfo pipelineInfo = vk::GraphicsPipelineCreateInfo()
        .setStageCount(2)
        .setPStages(shaderStages)
        .setPVertexInputState(&vertexInputInfo)
        .setPInputAssemblyState(&inputAssembly)
        .setPViewportState(&viewportState)
        .setPRasterizationState(&rasterizer)
        .setPMultisampleState(&multisampling)
        .setPDepthStencilState(&depthStencil)
        .setPColorBlendState(&colorBlending)
        .setPDynamicState(&dynamicState)
        .setLayout(ctx.pipeline.lightingPipelineLayout)
        .setRenderPass(ctx.pipeline.renderPass)
        .setSubpass(1)
        .setBasePipelineHandle(nullptr)
        .setBasePipelineIndex(-1);

    ctx.pipeline.lightingPipeline = ctx.device.createGraphicsPipeline(nullptr, pipelineInfo).value;

    ctx.device.destroyShaderModule(fragShaderModule);
    ctx.device.destroyShaderModule(vertShaderModule);

    return 0;
}

int vulkan_init_render_pass(VulkanContext& ctx)
{
    vk::AttachmentDescription positionAttachment = vk::AttachmentDescription()
        .setFormat(vk::Format::eR16G16B16A16Sfloat)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

    vk::AttachmentReference positionAttachmentRef = vk::AttachmentReference()
        .setAttachment(0)
        .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    vk::AttachmentDescription normalAttachment = vk::AttachmentDescription()
        .setFormat(vk::Format::eR16G16B16A16Sfloat)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

    vk::AttachmentReference normalAttachmentRef = vk::AttachmentReference()
        .setAttachment(1)
        .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    vk::AttachmentDescription albedoAttachment = vk::AttachmentDescription()
        .setFormat(vk::Format::eR8G8B8A8Unorm)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

    vk::AttachmentReference albedoAttachmentRef = vk::AttachmentReference()
        .setAttachment(2)
        .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    vk::AttachmentDescription colorAttachment = vk::AttachmentDescription()
        .setFormat(ctx.swapchain.swapchainFormat)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    vk::AttachmentReference colorAttachmentRef = vk::AttachmentReference()
        .setAttachment(3)
        .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    vk::AttachmentDescription depthAttachment = vk::AttachmentDescription()
        .setFormat(find_depth_format(ctx))
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::AttachmentReference depthAttachmentRef = vk::AttachmentReference()
        .setAttachment(4)
        .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    std::array<vk::AttachmentReference, 3> gBufferAttachmentsRefs = {
        positionAttachmentRef,
        normalAttachmentRef,
        albedoAttachmentRef
    };

    vk::SubpassDescription geometrySubpass = vk::SubpassDescription()
        .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
        .setColorAttachmentCount(3)
        .setPColorAttachments(gBufferAttachmentsRefs.data())
        .setPDepthStencilAttachment(&depthAttachmentRef);

    vk::AttachmentReference positionInputRef = vk::AttachmentReference()
        .setAttachment(0)
        .setLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

    vk::AttachmentReference normalInputRef = vk::AttachmentReference()
        .setAttachment(1)
        .setLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

    vk::AttachmentReference albedoInputRef = vk::AttachmentReference()
        .setAttachment(2)
        .setLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

    std::array<vk::AttachmentReference, 3> gBufferInputRefs = {
        positionInputRef,
        normalInputRef,
        albedoInputRef
    };

    vk::SubpassDescription lightingSubpass = vk::SubpassDescription()
        .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
        .setColorAttachmentCount(1)
        .setPColorAttachments(&colorAttachmentRef)
        .setInputAttachmentCount(3)
        .setPInputAttachments(gBufferInputRefs.data());

    std::array<vk::SubpassDependency, 2> dependencies{};
    dependencies[0]
        .setSrcSubpass(vk::SubpassExternal)
        .setDstSubpass(0)
        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput
            | vk::PipelineStageFlagBits::eLateFragmentTests)
        .setSrcAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite)
        .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput
            | vk::PipelineStageFlagBits::eEarlyFragmentTests)
        .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite
            | vk::AccessFlagBits::eDepthStencilAttachmentWrite);
    dependencies[1]
        .setSrcSubpass(0)
        .setDstSubpass(1)
        .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
        .setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
        .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
        .setDstAccessMask(vk::AccessFlagBits::eInputAttachmentRead);

    std::array<vk::AttachmentDescription, 5> attachments = {
        positionAttachment,
        normalAttachment,
        albedoAttachment,
        colorAttachment, 
        depthAttachment 
    };

    std::array<vk::SubpassDescription, 2> subpasses = {
        geometrySubpass,
        lightingSubpass
    };

    vk::RenderPassCreateInfo renderPassInfo = vk::RenderPassCreateInfo()
        .setAttachmentCount(static_cast<uint32_t>(attachments.size()))
        .setPAttachments(attachments.data())
        .setSubpassCount(static_cast<uint32_t>(subpasses.size()))
        .setPSubpasses(subpasses.data())
        .setDependencyCount(static_cast<uint32_t>(dependencies.size()))
        .setPDependencies(dependencies.data());

    ctx.pipeline.renderPass = ctx.device.createRenderPass(renderPassInfo);

    return 0;
}

int vulkan_init_descriptor_set_layout(VulkanContext& ctx)
{
    vk::DescriptorSetLayoutBinding uboLayoutBinding = vk::DescriptorSetLayoutBinding()
        .setBinding(0)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eUniformBuffer)
        .setStageFlags(vk::ShaderStageFlagBits::eVertex)
        .setPImmutableSamplers(nullptr);

    vk::DescriptorSetLayoutBinding samplerLayoutBinding = vk::DescriptorSetLayoutBinding()
        .setBinding(1)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
        .setStageFlags(vk::ShaderStageFlagBits::eFragment)
        .setPImmutableSamplers(nullptr);

    vk::DescriptorSetLayoutBinding lightUboLayoutBinding = vk::DescriptorSetLayoutBinding()
        .setBinding(2)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eUniformBuffer)
        .setStageFlags(vk::ShaderStageFlagBits::eFragment)
        .setPImmutableSamplers(nullptr);

    std::array<vk::DescriptorSetLayoutBinding, 3> bindings = {
        uboLayoutBinding,
        samplerLayoutBinding,
        lightUboLayoutBinding
    };

    vk::DescriptorSetLayoutCreateInfo layuotInfo = vk::DescriptorSetLayoutCreateInfo()
        .setBindingCount(static_cast<uint32_t>(bindings.size()))
        .setPBindings(bindings.data());

    ctx.pipeline.descriptorSetLayout = ctx.device.createDescriptorSetLayout(layuotInfo);

    return 0;
}

int vulkan_init_lighting_descriptor_set_layout(VulkanContext& ctx)
{
    vk::DescriptorSetLayoutBinding positionLayoutBinding = vk::DescriptorSetLayoutBinding()
        .setBinding(0)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eInputAttachment)
        .setStageFlags(vk::ShaderStageFlagBits::eFragment)
        .setPImmutableSamplers(nullptr);

    vk::DescriptorSetLayoutBinding normalLayoutBinding = vk::DescriptorSetLayoutBinding()
        .setBinding(1)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eInputAttachment)
        .setStageFlags(vk::ShaderStageFlagBits::eFragment)
        .setPImmutableSamplers(nullptr);

    vk::DescriptorSetLayoutBinding albedoLayoutBinding = vk::DescriptorSetLayoutBinding()
        .setBinding(2)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eInputAttachment)
        .setStageFlags(vk::ShaderStageFlagBits::eFragment)
        .setPImmutableSamplers(nullptr);

    vk::DescriptorSetLayoutBinding lightUboLayoutBinding = vk::DescriptorSetLayoutBinding()
        .setBinding(3)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eUniformBuffer)
        .setStageFlags(vk::ShaderStageFlagBits::eFragment)
        .setPImmutableSamplers(nullptr);

    std::array<vk::DescriptorSetLayoutBinding, 4> bindings = {
        positionLayoutBinding,
        normalLayoutBinding,
        albedoLayoutBinding,
        lightUboLayoutBinding
    };

    vk::DescriptorSetLayoutCreateInfo layuotInfo = vk::DescriptorSetLayoutCreateInfo()
        .setBindingCount(static_cast<uint32_t>(bindings.size()))
        .setPBindings(bindings.data());

    ctx.pipeline.lightingDescriptorSetLayout = ctx.device.createDescriptorSetLayout(layuotInfo);

    return 0;
}

vk::ShaderModule create_shader_module(VulkanContext& ctx, const std::vector<char>& code)
{
    vk::ShaderModuleCreateInfo createInfo = vk::ShaderModuleCreateInfo()
        .setCodeSize(code.size())
        .setPCode(reinterpret_cast<const uint32_t*>(code.data()));

    vk::ShaderModule shaderModule = ctx.device.createShaderModule(createInfo);

    return shaderModule;
}