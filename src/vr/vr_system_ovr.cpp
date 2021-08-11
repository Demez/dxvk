#include "vr_system_ovr.h"

#define BUTTON_DEADZONE  0.05f;

void Matrix_SetIdentity(float matrix[4][4])
{
	memset(matrix, 0, 16 * sizeof(float));
	matrix[0][0] = matrix[1][1] = matrix[2][2] = matrix[3][3] = 1.0f;
}

inline void Swap(float& a, float& b)
{
	float tmp = a;
	a = b;
	b = tmp;
}

void MatrixTranspose(float src[4][4], float dst[4][4])
{
	if (src == dst)
	{
		Swap(dst[0][1], dst[1][0]);
		Swap(dst[0][2], dst[2][0]);
		Swap(dst[0][3], dst[3][0]);
		Swap(dst[1][2], dst[2][1]);
		Swap(dst[1][3], dst[3][1]);
		Swap(dst[2][3], dst[3][2]);
	}
	else
	{
		dst[0][0] = src[0][0]; dst[0][1] = src[1][0]; dst[0][2] = src[2][0]; dst[0][3] = src[3][0];
		dst[1][0] = src[0][1]; dst[1][1] = src[1][1]; dst[1][2] = src[2][1]; dst[1][3] = src[3][1];
		dst[2][0] = src[0][2]; dst[2][1] = src[1][2]; dst[2][2] = src[2][2]; dst[2][3] = src[3][2];
		dst[3][0] = src[0][3]; dst[3][1] = src[1][3]; dst[3][2] = src[2][3]; dst[3][3] = src[3][3];
	}
}


BaseVRSystem* g_vrSystem = NULL;


BaseVRSystem* BaseVRSystem::Create()
{
	g_vrSystem = new OpenVRSystem;
	return g_vrSystem;
}


int GetVRSystem( IDXVK_VRSystem** system )
{
	IDXVK_VRSystem* baseSystem = g_vrSystem;
	*system = baseSystem;
	return 0;
}


/**
* Constructor.
***/ 
OpenVRSystem::OpenVRSystem() : 
	m_gameVR(NULL),
	m_initialised(false),
	m_bRTActive(false),
	m_nextStoredTexture(0),
	m_currentRenderTexture(-1),
	m_leftTexture(-1),
	m_rightTexture(-1),
	m_nRenderWidth(-1),
	m_nRenderHeight(-1),
	m_hasHMDAttached(true),
	m_posesStale(true)
{
	memset(m_SharedTextureHolder, 0, sizeof(m_SharedTextureHolder));
}

OpenVRSystem::~OpenVRSystem()
{
}


bool OpenVRSystem::Init( IVRInterface* system )
{
	m_gameVR = system;
	m_initialised = true;
	return true;
}

bool OpenVRSystem::Shutdown()
{
	m_initialised = false;
	return true;
}

bool OpenVRSystem::IsActive()
{
	return m_initialised;
}

bool OpenVRSystem::IsRenderTargetActive()
{
	return m_bRTActive;
}


VkSubmitThreadCallback* OpenVRSystem::GetVkSubmitThreadCallback()
{
	return this;
}


void OpenVRSystem::SetRenderTargetActive( bool active )
{
	m_bRTActive = active;
}

void OpenVRSystem::SetRenderTargetSize( uint32_t width, uint32_t height )
{
	m_nRenderWidth = width;
	m_nRenderHeight = height;
}

void OpenVRSystem::GetRenderTargetSize( uint32_t &width, uint32_t &height )
{
	width = m_nRenderWidth;
	height = m_nRenderHeight;
}


void OpenVRSystem::NextCreateTextureIsEye( vr::EVREye eye )
{
	m_nextRTIsEye = eye;
}


void OpenVRSystem::PrePresent()
{
	if (!m_initialised)
	{
		return;
	}

	{
		//Set atomic values
		{
			// m_submitTexture = m_currentRenderTexture;

			if ( m_leftTexture == -1 )
			{
				m_leftTexture = m_currentRenderTexture;
			}
			else
			{
				m_rightTexture = m_currentRenderTexture;
			}

			 if ( m_leftTexture != -1 && m_rightTexture != -1 )
				m_posesStale = true;

			// if ( m_currentRenderTexture == m_rightTexture )
			//	m_posesStale = true;
		}

		//Move to the next texture
		/*if (++m_currentRenderTexture > m_nextStoredTexture)
		{
			m_currentRenderTexture = 0;
		}*/

		// if ( m_currentRenderTexture != -1 )
		{
			if (++m_currentRenderTexture > 1)
			{
				m_currentRenderTexture = -1;
			}
		}
	}
}

void OpenVRSystem::PostPresent()
{
	if (!m_initialised)
	{
		return;
	}
}

void OpenVRSystem::PrePresentCallBack()
{
	if (!m_initialised)
	{
		return;
	}

	//Only do this if we know the PresentEx on the Device was called by the client
	if (m_leftTexture != -1 && m_rightTexture != -1)
	{
		vr::VRTextureBounds_t leftBounds = m_gameVR->GetTextureBounds( vr::Eye_Left );
		vr::VRTextureBounds_t rightBounds = m_gameVR->GetTextureBounds( vr::Eye_Right );

		vr::IVRCompositor* compositor = m_gameVR->GetCompositor();

		if (compositor && compositor->CanRenderScene())
		{
			m_gameVR->HandleSubmitError( compositor->Submit(vr::Eye_Left, &(m_SharedTextureHolder[m_leftTexture].m_VRTexture), &leftBounds) );
			m_gameVR->HandleSubmitError( compositor->Submit(vr::Eye_Right, &(m_SharedTextureHolder[m_rightTexture].m_VRTexture), &rightBounds) );
		}

		m_leftTexture = -1;
		m_rightTexture = -1;
		m_currentRenderTexture = -1;
	}
}

void OpenVRSystem::PostPresentCallback()
{
	if (!m_initialised)
	{
		return;
	}

	if (m_posesStale)
	{
		m_gameVR->WaitGetPoses();

		std::unique_lock<std::mutex> lock(m_mutex);

		m_posesStale = false;

		//Let main thread know poses are ready if it is waiting on them
		m_cv.notify_one();
	}
}


void OpenVRSystem::StartFrame()
{
	if (!m_initialised)
	{
		return;
	}

	{
		std::unique_lock<std::mutex> lock(m_mutex);

		if (m_posesStale)
		{
			m_cv.wait(lock);
		}
	}

	m_gameVR->UpdatePoses();
}

void OpenVRSystem::StoreSharedTexture(int index, vr::VRVulkanTextureData_t* pVRVulkanTextureData)
{
	// m_nextStoredTexture = index;
	m_currentRenderTexture++;

	if ( m_nextRTIsEye != -1 )
		m_currentRenderTexture = m_nextRTIsEye;

	m_nextStoredTexture = m_currentRenderTexture;

	// if (m_hasHMDAttached)
	{
		{
			memcpy(&m_SharedTextureHolder[m_nextStoredTexture].m_VulkanData, pVRVulkanTextureData, sizeof(vr::VRVulkanTextureData_t));

			m_SharedTextureHolder[m_nextStoredTexture].m_VRTexture.handle = &m_SharedTextureHolder[m_nextStoredTexture].m_VulkanData;
			m_SharedTextureHolder[m_nextStoredTexture].m_VRTexture.eColorSpace = vr::ColorSpace_Auto;
			m_SharedTextureHolder[m_nextStoredTexture].m_VRTexture.eType = vr::TextureType_Vulkan;
		}
	}

	if ( m_nextRTIsEye == 1 )
		m_nextRTIsEye = -1;

	if (m_currentRenderTexture > 0)
	{
		m_currentRenderTexture = -1;
	}
}

int OpenVRSystem::GetCurrentRenderTexture()
{
	return m_currentRenderTexture;
}

int OpenVRSystem::GetTotalStoredTextures()
{
	return m_nextStoredTexture+1;
}


