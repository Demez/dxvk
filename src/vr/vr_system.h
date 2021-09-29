#pragma once

#include "VkSubmitThreadCallback.h"
#include "../../client/vr_dxvk.h"
#include "d3d9.h"


class BaseVRSystem : public IDXVK_VRSystem
{
public:
	static BaseVRSystem* Create();

	virtual ~BaseVRSystem() {}

	virtual bool Init( IVRInterface* system ) = 0;
	virtual bool Shutdown() = 0;
	virtual bool IsActive() = 0;
	virtual bool IsRenderTargetActive() = 0;

	virtual VkSubmitThreadCallback* GetVkSubmitThreadCallback() = 0;

	virtual void SetRenderTargetActive( bool enabled ) = 0;
	virtual void SetRenderTargetSize( uint32_t width, uint32_t height ) = 0;
	virtual void GetRenderTargetSize( uint32_t &width, uint32_t &height ) = 0;
	virtual void NextCreateTextureIsEye( vr::EVREye eye ) = 0;

	virtual void PrePresent() = 0;
	virtual void PostPresent() = 0;
	virtual void StoreSharedTexture(int index, vr::VRVulkanTextureData_t* pVRVulkanTextureData) = 0;
	virtual int GetCurrentRenderTexture() = 0;
	virtual int GetTotalStoredTextures() = 0;

	virtual void SetMultiSampleEnabled( bool enabled ) = 0;
	virtual bool IsMultiSampleEnabled() = 0;

	virtual void StartFrame() = 0;
};

extern BaseVRSystem* g_vrSystem;
