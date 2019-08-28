#include <cg_base.hpp>

namespace cgb
{
	buffer_member_format format_4x_fp16()	{ return buffer_member_format{ vk::Format::eR16G16B16A16Sfloat }; }
	buffer_member_format format_3x_fp16()	{ return buffer_member_format{ vk::Format::eR16G16B16Sfloat }; }
	buffer_member_format format_2x_fp16()	{ return buffer_member_format{ vk::Format::eR16G16Sfloat }; }
	buffer_member_format format_1x_fp16()	{ return buffer_member_format{ vk::Format::eR16Sfloat }; }
	buffer_member_format format_4x_fp32()	{ return buffer_member_format{ vk::Format::eR32G32B32A32Sfloat }; }
	buffer_member_format format_3x_fp32()	{ return buffer_member_format{ vk::Format::eR32G32B32Sfloat }; }
	buffer_member_format format_2x_fp32()	{ return buffer_member_format{ vk::Format::eR32G32Sfloat }; }
	buffer_member_format format_1x_fp32()	{ return buffer_member_format{ vk::Format::eR32Sfloat }; }
	buffer_member_format format_4x_fp64()	{ return buffer_member_format{ vk::Format::eR64G64B64A64Sfloat }; }
	buffer_member_format format_3x_fp64()	{ return buffer_member_format{ vk::Format::eR64G64B64Sfloat }; }
	buffer_member_format format_2x_fp64()	{ return buffer_member_format{ vk::Format::eR64G64Sfloat }; }
	buffer_member_format format_1x_fp64()	{ return buffer_member_format{ vk::Format::eR64Sfloat }; }
	buffer_member_format format_4x_sint8()	{ return buffer_member_format{ vk::Format::eR8G8B8A8Sint }; }
	buffer_member_format format_3x_sint8()	{ return buffer_member_format{ vk::Format::eR8G8B8Sint }; }
	buffer_member_format format_2x_sint8()	{ return buffer_member_format{ vk::Format::eR8G8Sint }; }
	buffer_member_format format_1x_sint8()	{ return buffer_member_format{ vk::Format::eR8Sint }; }
	buffer_member_format format_4x_sint16()	{ return buffer_member_format{ vk::Format::eR16G16B16A16Sint }; }
	buffer_member_format format_3x_sint16()	{ return buffer_member_format{ vk::Format::eR16G16B16Sint }; }
	buffer_member_format format_2x_sint16()	{ return buffer_member_format{ vk::Format::eR16G16Sint }; }
	buffer_member_format format_1x_sint16()	{ return buffer_member_format{ vk::Format::eR16Sint }; }
	buffer_member_format format_4x_sint32()	{ return buffer_member_format{ vk::Format::eR32G32B32A32Sint }; }
	buffer_member_format format_3x_sint32()	{ return buffer_member_format{ vk::Format::eR32G32B32Sint }; }
	buffer_member_format format_2x_sint32()	{ return buffer_member_format{ vk::Format::eR32G32Sint }; }
	buffer_member_format format_1x_sint32()	{ return buffer_member_format{ vk::Format::eR32Sint }; }
	buffer_member_format format_4x_sint64()	{ return buffer_member_format{ vk::Format::eR64G64B64A64Sint }; }
	buffer_member_format format_3x_sint64()	{ return buffer_member_format{ vk::Format::eR64G64B64Sint }; }
	buffer_member_format format_2x_sint64()	{ return buffer_member_format{ vk::Format::eR64G64Sint }; }
	buffer_member_format format_1x_sint64()	{ return buffer_member_format{ vk::Format::eR64Sint }; }
	buffer_member_format format_4x_uint8()	{ return buffer_member_format{ vk::Format::eR8G8B8A8Uint }; }
	buffer_member_format format_3x_uint8()	{ return buffer_member_format{ vk::Format::eR8G8B8Uint }; }
	buffer_member_format format_2x_uint8()	{ return buffer_member_format{ vk::Format::eR8G8Uint }; }
	buffer_member_format format_1x_uint8()	{ return buffer_member_format{ vk::Format::eR8Uint }; }
	buffer_member_format format_4x_uint16()	{ return buffer_member_format{ vk::Format::eR16G16B16A16Uint }; }
	buffer_member_format format_3x_uint16()	{ return buffer_member_format{ vk::Format::eR16G16B16Uint }; }
	buffer_member_format format_2x_uint16()	{ return buffer_member_format{ vk::Format::eR16G16Uint }; }
	buffer_member_format format_1x_uint16()	{ return buffer_member_format{ vk::Format::eR16Uint }; }
	buffer_member_format format_4x_uint32()	{ return buffer_member_format{ vk::Format::eR32G32B32A32Uint }; }
	buffer_member_format format_3x_uint32()	{ return buffer_member_format{ vk::Format::eR32G32B32Uint }; }
	buffer_member_format format_2x_uint32()	{ return buffer_member_format{ vk::Format::eR32G32Uint }; }
	buffer_member_format format_1x_uint32()	{ return buffer_member_format{ vk::Format::eR32Uint }; }
	buffer_member_format format_4x_uint64()	{ return buffer_member_format{ vk::Format::eR64G64B64A64Uint }; }
	buffer_member_format format_3x_uint64()	{ return buffer_member_format{ vk::Format::eR64G64B64Uint }; }
	buffer_member_format format_2x_uint64()	{ return buffer_member_format{ vk::Format::eR64G64Uint }; }
	buffer_member_format format_1x_uint64()	{ return buffer_member_format{ vk::Format::eR64Uint }; }
}