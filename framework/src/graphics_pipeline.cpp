#include <cg_base.hpp>

namespace cgb
{
	using namespace cpplinq;
	using namespace cgb::cfg;

	owning_resource<graphics_pipeline_t> graphics_pipeline_t::create(graphics_pipeline_config aConfig, cgb::context_specific_function<void(graphics_pipeline_t&)> aAlterConfigBeforeCreation)
	{
		graphics_pipeline_t result;

		// 0. Own the renderpass
		{
			assert(aConfig.mRenderPassSubpass.has_value());
			auto [rp, sp] = std::move(aConfig.mRenderPassSubpass.value());
			result.mRenderPass = std::move(rp);
			result.mSubpassIndex = sp;
		}

		// 1. Compile the array of vertex input binding descriptions
		{ 
			// Select DISTINCT bindings:
			auto bindings = from(aConfig.mInputBindingLocations)
				>> select([](const input_binding_location_data& _BindingData) { return _BindingData.mGeneralData; })
				>> distinct() // see what I did there
				>> orderby([](const input_binding_general_data& _GeneralData) { return _GeneralData.mBinding; })
				>> to_vector();
			result.mVertexInputBindingDescriptions.reserve(bindings.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!

			for (auto& bindingData : bindings) {

				const auto numRecordsWithSameBinding = std::count_if(std::begin(bindings), std::end(bindings), 
					[bindingId = bindingData.mBinding](const input_binding_general_data& _GeneralData) {
						return _GeneralData.mBinding == bindingId;
					});
				if (1 != numRecordsWithSameBinding) {
					throw cgb::runtime_error(fmt::format("The input binding {} is defined in different ways. Make sure to define it uniformly across different bindings/attribute descriptions!", bindingData.mBinding));
				}

				result.mVertexInputBindingDescriptions.push_back(vk::VertexInputBindingDescription()
					// The following parameters are guaranteed to be the same. We have checked this.
					.setBinding(bindingData.mBinding)
					.setStride(static_cast<uint32_t>(bindingData.mStride))
					.setInputRate(to_vk_vertex_input_rate(bindingData.mKind))
					// Don't need the location here
				);
			}
		}

		// 2. Compile the array of vertex input attribute descriptions
		//  They will reference the bindings created in step 1.
		result.mVertexInputAttributeDescriptions.reserve(aConfig.mInputBindingLocations.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!
		for (auto& attribData : aConfig.mInputBindingLocations) {
			result.mVertexInputAttributeDescriptions.push_back(vk::VertexInputAttributeDescription()
				.setBinding(attribData.mGeneralData.mBinding)
				.setLocation(attribData.mMemberMetaData.mLocation)
				.setFormat(attribData.mMemberMetaData.mFormat.mFormat)
				.setOffset(static_cast<uint32_t>(attribData.mMemberMetaData.mOffset))
			);
		}

		// 3. With the data from 1. and 2., create the complete vertex input info struct, passed to the pipeline creation
		result.mPipelineVertexInputStateCreateInfo = vk::PipelineVertexInputStateCreateInfo()
			.setVertexBindingDescriptionCount(static_cast<uint32_t>(result.mVertexInputBindingDescriptions.size()))
			.setPVertexBindingDescriptions(result.mVertexInputBindingDescriptions.data())
			.setVertexAttributeDescriptionCount(static_cast<uint32_t>(result.mVertexInputAttributeDescriptions.size()))
			.setPVertexAttributeDescriptions(result.mVertexInputAttributeDescriptions.data());

		// 4. Set how the data (from steps 1.-3.) is to be interpreted (e.g. triangles, points, lists, patches, etc.)
		result.mInputAssemblyStateCreateInfo = vk::PipelineInputAssemblyStateCreateInfo()
			.setTopology(to_vk_primitive_topology(aConfig.mPrimitiveTopology))
			.setPrimitiveRestartEnable(VK_FALSE);

		// 5. Compile and store the shaders:
		result.mShaders.reserve(aConfig.mShaderInfos.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!
		result.mShaderStageCreateInfos.reserve(aConfig.mShaderInfos.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!
		for (auto& shaderInfo : aConfig.mShaderInfos) {
			// 5.0 Sanity check
			if (result.mShaders.end() != std::find_if(std::begin(result.mShaders), std::end(result.mShaders), [&shaderInfo](const shader& existing) { return existing.info().mShaderType == shaderInfo.mShaderType; })) {
				throw cgb::runtime_error(fmt::format("There's already a {}-type shader contained in this graphics pipeline. Can not add another one of the same type.", to_string(to_vk_shader_stages(shaderInfo.mShaderType))));
			}
			// 5.1 Compile the shader
			result.mShaders.push_back(shader::create(shaderInfo));
			assert(result.mShaders.back().has_been_built());
			// 5.2 Combine
			result.mShaderStageCreateInfos.push_back(vk::PipelineShaderStageCreateInfo{}
				.setStage(to_vk_shader_stage(result.mShaders.back().info().mShaderType))
				.setModule(result.mShaders.back().handle())
				.setPName(result.mShaders.back().info().mEntryPoint.c_str())
			);
		}

		// 6. Viewport configuration
		{
			// 6.1 Viewport and depth configuration(s):
			result.mViewports.reserve(aConfig.mViewportDepthConfig.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!
			result.mScissors.reserve(aConfig.mViewportDepthConfig.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!
			for (auto& vp : aConfig.mViewportDepthConfig) {
				result.mViewports.push_back(vk::Viewport{}
					.setX(vp.x())
					.setY(vp.y())
					.setWidth(vp.width())
					.setHeight(vp.height())
					.setMinDepth(vp.min_depth())
					.setMaxDepth(vp.max_depth())
				);
				// 6.2 Skip scissors for now
				// TODO: Implement scissors support properly
				result.mScissors.push_back(vk::Rect2D{}
					.setOffset({static_cast<int32_t>(vp.x()), static_cast<int32_t>(vp.y())})
					.setExtent({static_cast<uint32_t>(vp.width()), static_cast<uint32_t>(vp.height())})
				);
			}
			// 6.3 Add everything together
			result.mViewportStateCreateInfo = vk::PipelineViewportStateCreateInfo{}
				.setViewportCount(static_cast<uint32_t>(result.mViewports.size()))
				.setPViewports(result.mViewports.data())
				.setScissorCount(static_cast<uint32_t>(result.mScissors.size()))
				.setPScissors(result.mScissors.data());
		}

		// 7. Rasterization state
		result.mRasterizationStateCreateInfo =  vk::PipelineRasterizationStateCreateInfo{}
			// Various, but important settings:
			.setRasterizerDiscardEnable(to_vk_bool(aConfig.mRasterizerGeometryMode == rasterizer_geometry_mode::discard_geometry))
			.setPolygonMode(to_vk_polygon_mode(aConfig.mPolygonDrawingModeAndConfig.drawing_mode()))
			.setLineWidth(aConfig.mPolygonDrawingModeAndConfig.line_width())
			.setCullMode(to_vk_cull_mode(aConfig.mCullingMode))
			.setFrontFace(to_vk_front_face(aConfig.mFrontFaceWindingOrder.winding_order_of_front_faces()))
			// Depth-related settings:
			.setDepthClampEnable(to_vk_bool(aConfig.mDepthClampBiasConfig.is_clamp_to_frustum_enabled()))
			.setDepthBiasEnable(to_vk_bool(aConfig.mDepthClampBiasConfig.is_depth_bias_enabled()))
			.setDepthBiasConstantFactor(aConfig.mDepthClampBiasConfig.bias_constant_factor())
			.setDepthBiasClamp(aConfig.mDepthClampBiasConfig.bias_clamp_value())
			.setDepthBiasSlopeFactor(aConfig.mDepthClampBiasConfig.bias_slope_factor());

		// 8. Depth-stencil config
		result.mDepthStencilConfig = vk::PipelineDepthStencilStateCreateInfo{}
			.setDepthTestEnable(to_vk_bool(aConfig.mDepthTestConfig.is_enabled()))
			.setDepthCompareOp(to_vk_compare_op(aConfig.mDepthTestConfig.depth_compare_operation()))
			.setDepthWriteEnable(to_vk_bool(aConfig.mDepthWriteConfig.is_enabled()))
			.setDepthBoundsTestEnable(to_vk_bool(aConfig.mDepthBoundsConfig.is_enabled()))
			.setMinDepthBounds(aConfig.mDepthBoundsConfig.min_bounds())
			.setMaxDepthBounds(aConfig.mDepthBoundsConfig.max_bounds())
			.setStencilTestEnable(VK_FALSE);

		// TODO: Add better support for stencil testing (better abstraction!)
		if (aConfig.mStencilTest.has_value() && aConfig.mStencilTest.value().mEnabled) {
			result.mDepthStencilConfig
				.setStencilTestEnable(VK_TRUE)
				.setFront(aConfig.mStencilTest.value().mFrontStencilTestActions)
				.setBack(aConfig.mStencilTest.value().mBackStencilTestActions);
		}

		// 9. Color Blending
		{ 
			// Do we have an "universal" color blending config? That means, one that is not assigned to a specific color target attachment id.
			auto universalConfig = from(aConfig.mColorBlendingPerAttachment)
				>> where([](const color_blending_config& config) { return !config.mTargetAttachment.has_value(); })
				>> to_vector();

			if (universalConfig.size() > 1) {
				throw cgb::runtime_error("Ambiguous 'universal' color blending configurations. Either provide only one 'universal' "
					"config (which is not attached to a specific color target) or assign them to specific color target attachment ids.");
			}

			// Iterate over all color target attachments and set a color blending config
			if (result.subpass_id() >= result.mRenderPass->attachment_descriptions().size()) {
				throw cgb::runtime_error(fmt::format("There are fewer subpasses in the renderpass ({}) as the subpass index indicates ({}). I.e. subpass index is out of bounds.", result.mRenderPass->attachment_descriptions().size(), result.subpass_id()));
			}
			const auto n = result.mRenderPass->color_attachments_for_subpass(result.subpass_id()).size(); /////////////////// TODO: (doublecheck or) FIX this section (after renderpass refactoring)
			result.mBlendingConfigsForColorAttachments.reserve(n); // Important! Otherwise the vector might realloc and .data() will become invalid!
			for (size_t i = 0; i < n; ++i) {
				// Do we have a specific blending config for color attachment i?
				auto configForI = from(aConfig.mColorBlendingPerAttachment)
					>> where([i](const color_blending_config& config) { return config.mTargetAttachment.has_value() && config.mTargetAttachment.value() == i; })
					>> to_vector();
				if (configForI.size() > 1) {
					throw cgb::runtime_error(fmt::format("Ambiguous color blending configuration for color attachment at index {}. Provide only one config per color attachment!", i));
				}
				// Determine which color blending to use for this attachment:
				color_blending_config toUse = configForI.size() == 1 ? configForI[0] : color_blending_config::disable();
				result.mBlendingConfigsForColorAttachments.push_back(vk::PipelineColorBlendAttachmentState()
					.setColorWriteMask(to_vk_color_components(toUse.affected_color_channels()))
					.setBlendEnable(to_vk_bool(toUse.is_blending_enabled())) // If blendEnable is set to VK_FALSE, then the new color from the fragment shader is passed through unmodified. [4]
					.setSrcColorBlendFactor(to_vk_blend_factor(toUse.color_source_factor())) 
					.setDstColorBlendFactor(to_vk_blend_factor(toUse.color_destination_factor()))
					.setColorBlendOp(to_vk_blend_operation(toUse.color_operation()))
					.setSrcAlphaBlendFactor(to_vk_blend_factor(toUse.alpha_source_factor()))
					.setDstAlphaBlendFactor(to_vk_blend_factor(toUse.alpha_destination_factor()))
					.setAlphaBlendOp(to_vk_blend_operation(toUse.alpha_operation()))
				);
			}

			// General blending settings and reference to the array of color attachment blending configs
			result.mColorBlendStateCreateInfo = vk::PipelineColorBlendStateCreateInfo()
				.setLogicOpEnable(to_vk_bool(aConfig.mColorBlendingSettings.is_logic_operation_enabled())) // If you want to use the second method of blending (bitwise combination), then you should set logicOpEnable to VK_TRUE. The bitwise operation can then be specified in the logicOp field. [4]
				.setLogicOp(to_vk_logic_operation(aConfig.mColorBlendingSettings.logic_operation())) 
				.setAttachmentCount(static_cast<uint32_t>(result.mBlendingConfigsForColorAttachments.size()))
				.setPAttachments(result.mBlendingConfigsForColorAttachments.data())
				.setBlendConstants({
					{
						aConfig.mColorBlendingSettings.blend_constants().r,
						aConfig.mColorBlendingSettings.blend_constants().g,
						aConfig.mColorBlendingSettings.blend_constants().b,
						aConfig.mColorBlendingSettings.blend_constants().a,
					} 
				});
		}

		// 10. Multisample state
		// TODO: Can the settings be inferred from the renderpass' color attachments (as they are right now)? If they can't, how to handle this situation? 
		{ /////////////////// TODO: FIX this section (after renderpass refactoring)
			vk::SampleCountFlagBits numSamples = vk::SampleCountFlagBits::e1;

			// See what is configured in the render pass
			auto colorAttConfigs = from ((*result.mRenderPass).color_attachments_for_subpass(result.subpass_id()))
				>> where ([](const vk::AttachmentReference& colorAttachment) { return colorAttachment.attachment != VK_ATTACHMENT_UNUSED; })
				// The color_attachments() contain indices of the actual attachment_descriptions() => select the latter!
				>> select ([&rp = (*result.mRenderPass)](const vk::AttachmentReference& colorAttachment) { return rp.attachment_descriptions()[colorAttachment.attachment]; })
				>> to_vector();

			for (const vk::AttachmentDescription& config: colorAttConfigs) {
				typedef std::underlying_type<vk::SampleCountFlagBits>::type EnumType;
				numSamples = static_cast<vk::SampleCountFlagBits>(std::max(static_cast<EnumType>(config.samples), static_cast<EnumType>(numSamples)));
			}

#if defined(_DEBUG) 
			for (const vk::AttachmentDescription& config: colorAttConfigs) {
				if (config.samples != numSamples) {
					LOG_DEBUG("Not all of the color target attachments have the same number of samples configured, fyi. This might be fine, though.");
				}
			}
#endif
			
			if (vk::SampleCountFlagBits::e1 == numSamples) {
				auto depthAttConfigs = from ((*result.mRenderPass).depth_stencil_attachments_for_subpass(result.subpass_id()))
					>> where ([](const vk::AttachmentReference& depthStencilAttachment) { return depthStencilAttachment.attachment != VK_ATTACHMENT_UNUSED; })
					>> select ([&rp = (*result.mRenderPass)](const vk::AttachmentReference& depthStencilAttachment) { return rp.attachment_descriptions()[depthStencilAttachment.attachment]; })
					>> to_vector();

				for (const vk::AttachmentDescription& config: depthAttConfigs) {
					typedef std::underlying_type<vk::SampleCountFlagBits>::type EnumType;
					numSamples = static_cast<vk::SampleCountFlagBits>(std::max(static_cast<EnumType>(config.samples), static_cast<EnumType>(numSamples)));
				}

#if defined(_DEBUG) 
					for (const vk::AttachmentDescription& config: depthAttConfigs) {
						if (config.samples != numSamples) {
							LOG_DEBUG("Not all of the depth/stencil target attachments have the same number of samples configured, fyi. This might be fine, though.");
						}
					}
#endif

#if defined(_DEBUG) 
					for (const vk::AttachmentDescription& config: colorAttConfigs) {
						if (config.samples != numSamples) {
							LOG_DEBUG("Some of the color target attachments have different numbers of samples configured as the depth/stencil attachments, fyi. This might be fine, though.");
						}
					}
#endif
			}
			
			// Evaluate and set the PER SAMPLE shading configuration:
			auto perSample = aConfig.mPerSampleShading.value_or(per_sample_shading_config{ false, 1.0f });
			
			result.mMultisampleStateCreateInfo = vk::PipelineMultisampleStateCreateInfo()
				.setRasterizationSamples(numSamples)
				.setSampleShadingEnable(perSample.mPerSampleShadingEnabled ? VK_TRUE : VK_FALSE) // enable/disable Sample Shading
				.setMinSampleShading(perSample.mMinFractionOfSamplesShaded) // specifies a minimum fraction of sample shading if sampleShadingEnable is set to VK_TRUE.
				.setPSampleMask(nullptr) // If pSampleMask is NULL, it is treated as if the mask has all bits enabled, i.e. no coverage is removed from fragments. See https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#fragops-samplemask
				.setAlphaToCoverageEnable(VK_FALSE) // controls whether a temporary coverage value is generated based on the alpha component of the fragment�s first color output as specified in the Multisample Coverage section.
				.setAlphaToOneEnable(VK_FALSE); // controls whether the alpha component of the fragment�s first color output is replaced with one as described in Multisample Coverage.
			// TODO: That is probably not enough for every case. Further customization options should be added!
		}

		// 11. Dynamic state
		{
			// Don't need to pre-alloc the storage for this one

			// Check for viewport dynamic state
			for (const auto& vpdc : aConfig.mViewportDepthConfig) {
				if (vpdc.is_dynamic_viewport_enabled())	{
					result.mDynamicStateEntries.push_back(vk::DynamicState::eViewport);
				}
			}
			// Check for scissor dynamic state
			for (const auto& vpdc : aConfig.mViewportDepthConfig) {
				if (vpdc.is_dynamic_scissor_enabled())	{
					result.mDynamicStateEntries.push_back(vk::DynamicState::eScissor);
				}
			}
			// Check for dynamic line width
			if (aConfig.mPolygonDrawingModeAndConfig.dynamic_line_width()) {
				result.mDynamicStateEntries.push_back(vk::DynamicState::eLineWidth);
			}
			// Check for dynamic depth bias
			if (aConfig.mDepthClampBiasConfig.is_dynamic_depth_bias_enabled()) {
				result.mDynamicStateEntries.push_back(vk::DynamicState::eDepthBias);
			}
			// Check for dynamic depth bounds
			if (aConfig.mDepthBoundsConfig.is_dynamic_depth_bounds_enabled()) {
				result.mDynamicStateEntries.push_back(vk::DynamicState::eDepthBounds);
			}
			// Check for dynamic stencil values // TODO: make them configurable separately
			if (aConfig.mStencilTest.has_value() && aConfig.mStencilTest.value().is_dynamic_enabled()) {
				result.mDynamicStateEntries.push_back(vk::DynamicState::eStencilCompareMask);
				result.mDynamicStateEntries.push_back(vk::DynamicState::eStencilReference);
				result.mDynamicStateEntries.push_back(vk::DynamicState::eStencilWriteMask);
			}
			// TODO: Support further dynamic states

			result.mDynamicStateCreateInfo = vk::PipelineDynamicStateCreateInfo{}
				.setDynamicStateCount(static_cast<uint32_t>(result.mDynamicStateEntries.size()))
				.setPDynamicStates(result.mDynamicStateEntries.data());
		}

		// 12. Flags
		// TODO: Support all flags (only one of the flags is handled at the moment)
		result.mPipelineCreateFlags = {};
		if ((aConfig.mPipelineSettings & pipeline_settings::disable_optimization) == pipeline_settings::disable_optimization) {
			result.mPipelineCreateFlags |= vk::PipelineCreateFlagBits::eDisableOptimization;
		}

		// 13. Patch Control Points for Tessellation
		if (aConfig.mTessellationPatchControlPoints.has_value()) {
			result.mPipelineTessellationStateCreateInfo = vk::PipelineTessellationStateCreateInfo{}
				.setPatchControlPoints(aConfig.mTessellationPatchControlPoints.value().mPatchControlPoints);
		}

		// 14. Compile the PIPELINE LAYOUT data and create-info
		// Get the descriptor set layouts
		result.mAllDescriptorSetLayouts = set_of_descriptor_set_layouts::prepare(std::move(aConfig.mResourceBindings));
		result.mAllDescriptorSetLayouts.allocate_all();
		auto descriptorSetLayoutHandles = result.mAllDescriptorSetLayouts.layout_handles();
		// Gather the push constant data
		result.mPushConstantRanges.reserve(aConfig.mPushConstantsBindings.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!
		for (const auto& pcBinding : aConfig.mPushConstantsBindings) {
			result.mPushConstantRanges.push_back(vk::PushConstantRange{}
				.setStageFlags(to_vk_shader_stages(pcBinding.mShaderStages))
				.setOffset(static_cast<uint32_t>(pcBinding.mOffset))
				.setSize(static_cast<uint32_t>(pcBinding.mSize))
			);
			// TODO: Push Constants need a prettier interface
		}
		// These uniform values (Anm.: passed to shaders) need to be specified during pipeline creation by creating a VkPipelineLayout object. [4]
		result.mPipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo{}
			.setSetLayoutCount(static_cast<uint32_t>(descriptorSetLayoutHandles.size()))
			.setPSetLayouts(descriptorSetLayoutHandles.data())
			.setPushConstantRangeCount(static_cast<uint32_t>(result.mPushConstantRanges.size())) 
			.setPPushConstantRanges(result.mPushConstantRanges.data());

		// 15. Maybe alter the config?!
		if (aAlterConfigBeforeCreation.mFunction) {
			aAlterConfigBeforeCreation.mFunction(result);
		}

		// Create the PIPELINE LAYOUT
		result.mPipelineLayout = context().logical_device().createPipelineLayoutUnique(result.mPipelineLayoutCreateInfo);
		assert(nullptr != result.layout_handle());

		assert (aConfig.mRenderPassSubpass.has_value());
		// Create the PIPELINE, a.k.a. putting it all together:
		auto pipelineInfo = vk::GraphicsPipelineCreateInfo{}
			// 0. Render Pass
			.setRenderPass((*result.mRenderPass).handle())
			.setSubpass(result.mSubpassIndex)
			// 1., 2., and 3.
			.setPVertexInputState(&result.mPipelineVertexInputStateCreateInfo)
			// 4.
			.setPInputAssemblyState(&result.mInputAssemblyStateCreateInfo)
			// 5.
			.setStageCount(static_cast<uint32_t>(result.mShaderStageCreateInfos.size()))
			.setPStages(result.mShaderStageCreateInfos.data())
			// 6.
			.setPViewportState(&result.mViewportStateCreateInfo)
			// 7.
			.setPRasterizationState(&result.mRasterizationStateCreateInfo)
			// 8.
			.setPDepthStencilState(&result.mDepthStencilConfig)
			// 9.
			.setPColorBlendState(&result.mColorBlendStateCreateInfo)
			// 10.
			.setPMultisampleState(&result.mMultisampleStateCreateInfo)
			// 11.
			.setPDynamicState(result.mDynamicStateEntries.size() == 0 ? nullptr : &result.mDynamicStateCreateInfo) // Optional
			// 12.
			.setFlags(result.mPipelineCreateFlags)
			// LAYOUT:
			.setLayout(result.layout_handle())
			// Base pipeline:
			.setBasePipelineHandle(nullptr) // Optional
			.setBasePipelineIndex(-1); // Optional

		// 13.
		if (result.mPipelineTessellationStateCreateInfo.has_value()) {
			pipelineInfo.setPTessellationState(&result.mPipelineTessellationStateCreateInfo.value());
		}

		// TODO: Shouldn't the config be altered HERE, after the pipelineInfo has been compiled?!
		
		result.mPipeline = context().logical_device().createGraphicsPipelineUnique(nullptr, pipelineInfo);
		result.mTracker.setTrackee(result);
		return result;
	}


}