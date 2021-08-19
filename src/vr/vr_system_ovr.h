#pragma once

// what is this for?
#pragma warning (disable : 4005)

#include "vr_system.h"
#include "VkSubmitThreadCallback.h"

#include <mutex>
#include <condition_variable>
#include <atomic>


struct SharedTextureHolder
{
	vr::VRVulkanTextureData_t m_VulkanData;
	vr::Texture_t			m_VRTexture;
};


class OpenVRSystem : public BaseVRSystem,
	public VkSubmitThreadCallback
{
public:
	OpenVRSystem();
	virtual ~OpenVRSystem();

	virtual bool Init( IVRInterface* system );	
	virtual bool Shutdown();
	virtual bool IsActive();
	virtual bool IsRenderTargetActive();

	virtual VkSubmitThreadCallback* GetVkSubmitThreadCallback();

	// VkSubmitThreadCallback
	virtual void PrePresentCallBack();
	virtual void PostPresentCallback();

	virtual void SetRenderTargetActive( bool enabled );
	virtual void SetRenderTargetSize( uint32_t width, uint32_t height );
	virtual void GetRenderTargetSize( uint32_t &width, uint32_t &height );

	virtual void NextCreateTextureIsEye( vr::EVREye eye );

	//HMDInterface
	virtual void PrePresent();
	virtual void PostPresent();
	virtual void StoreSharedTexture(int index, vr::VRVulkanTextureData_t* pVRVulkanTextureData);
	virtual int GetCurrentRenderTexture();
	virtual int GetTotalStoredTextures();

	virtual void StartFrame();

private:
	SharedTextureHolder			m_SharedTextureHolder[2];

	uint32_t m_nRenderWidth;
	uint32_t m_nRenderHeight;

	IVRInterface* m_gameVR;

private:
	bool m_initialised;
	int m_nextStoredTexture;
	int m_currentRenderTexture;
	bool m_hasHMDAttached;
	bool m_bRTActive;
	int m_nextRTIsEye;

	std::atomic<int>   m_leftTexture = { -1 };
	std::atomic<int>   m_rightTexture = { -1 };

	std::atomic<bool>   m_posesStale = { true };

	std::mutex m_mutex;
	std::condition_variable m_cv;
};

