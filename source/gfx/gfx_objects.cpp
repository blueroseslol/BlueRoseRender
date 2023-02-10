#include <vector>
#include <iostream>
#include <assert.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include "gfx/gfx_objects.h"
#include "framework/common.h"

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")

bool LittleGFXInstance::Initialize(bool enableDebugLayer)
{
    debugLayerEnabled = enableDebugLayer;
    UINT flags = 0;
    if (debugLayerEnabled)
    {
#if defined(_DEBUG)
        flags = DXGI_CREATE_FACTORY_DEBUG;
        //启用调试层。在使用DXGI或D3D API之前，调试层应当首先被启用。在创建ID3D12Deviece后启用调试层会让device被移除。
        {
            Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
            {
                debugController->EnableDebugLayer();
            }
        }
#endif 
    }

    if (SUCCEEDED(CreateDXGIFactory2(flags, IID_PPV_ARGS(&pDXGIFactory))))
    {
        queryAllAdapters();
        // If the only adapter we found is a software adapter, log error message for QA
        if (!adapters.size() && foundSoftwareAdapter)
        {
            assert(0 && "The only available GPU has DXGI_ADAPTER_FLAG_SOFTWARE. Early exiting");
            return false;
        }
    }
    else
    {
        assert("[D3D12 Fatal]: Create DXGIFactory2 Failed!");
        return false;
    }
    return true;
}

bool LittleGFXInstance::Destroy()
{
    for (auto iter : adapters)
    {
        SAFE_RELEASE(iter.pDXGIAdapter);
    }
    SAFE_RELEASE(pDXGIFactory);
    return true;
}

void LittleGFXInstance::queryAllAdapters()
{
    IDXGIAdapter4* adapter = NULL;
    // Use DXGI6 interface which lets us specify gpu preference so we dont need to use NVOptimus or AMDPowerExpress
    for (UINT i = 0;
         pDXGIFactory->EnumAdapterByGpuPreference(i,
             DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
             IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND;
         i++)
    {
        LittleGFXAdapter newAdapter;
        newAdapter.pDXGIAdapter = adapter;
        newAdapter.instance = this;
        DXGI_ADAPTER_DESC3 desc = {};
        adapter->GetDesc3(&desc);
        std::wcout << desc.Description << std::endl;
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            foundSoftwareAdapter = true;
            SAFE_RELEASE(adapter);
        }
        else
        {
            adapters.emplace_back(newAdapter);
        }
    }
}

/*
//使用案例
D3D_FEATURE_LEVEL featureLevels[] =
{
    D3D_FEATURE_LEVEL_12_2,
    D3D_FEATURE_LEVEL_12_1,
    D3D_FEATURE_LEVEL_12_0,
    D3D_FEATURE_LEVEL_11_1,
    D3D_FEATURE_LEVEL_11_0,
};

LittleGFXAdapter* GFXAdapter=nullptr;
for (std::uint32_t i = 0U; i < _countof(featureLevels); ++i)
{
    GFXAdapter=instance->GetSupportedAdapter(featureLevels[i]);
}
*/

LittleGFXAdapter* LittleGFXInstance::GetSupportedAdapter(const D3D_FEATURE_LEVEL featureLevel,const uint32_t idx)
{
    if (pDXGIFactory == nullptr)
    {
        throw std::runtime_error(std::string("DXGIFactory Is Nullptr."));
    }
    
    for (std::uint32_t adapterIndex = 0U; ; ++adapterIndex) 
    {
        Microsoft::WRL::ComPtr<IDXGIAdapter4> dxgiAdapter4;
        Microsoft::WRL::ComPtr<IDXGIAdapter1> dxgiAdapter1;
        if (DXGI_ERROR_NOT_FOUND == pDXGIFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1))) 
        {
            break;
        }
    
        const HRESULT hres = D3D12CreateDevice(dxgiAdapter1.Get(),featureLevel,_uuidof(ID3D12Device),nullptr);
        if (SUCCEEDED(hres)) 
        {
            //ComPtr转换
            ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));

            LittleGFXAdapter newAdapter;
            newAdapter.pDXGIAdapter = dxgiAdapter4.Get();
            newAdapter.instance = this;
            adapters.emplace_back(newAdapter);
            break;
        }
        //dxgiAdapter1->Release();
    }
    
    assert(adapters.size() <= 0);
    return (idx+1) <= adapters.size() ? &adapters[idx] : &adapters[0];
}

bool LittleGFXDevice::Initialize(LittleGFXAdapter* in_adapter)
{
    this->adapter = in_adapter;
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_12_0;
    if (!SUCCEEDED(D3D12CreateDevice(adapter->pDXGIAdapter, // default adapter
            featureLevel, IID_PPV_ARGS(&pD3D12Device))))
    {
        assert(0 && "[D3D12 Fatal]: Create D3D12Device Failed!");
        return false;
    }
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    if (!SUCCEEDED(pD3D12Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pD3D12Queue))))
    {
        assert(0 && "[D3D12 Fatal]: Create D3D12CommandQueue Failed!");
        return false;
    }
    return true;
}

bool LittleGFXDevice::Destroy()
{
    SAFE_RELEASE(pD3D12Queue);
    SAFE_RELEASE(pD3D12Device);
    return true;
}

bool LittleGFXWindow::Initialize(const wchar_t* title, LittleGFXDevice* device, bool enableVsync)
{
    auto succeed = LittleWindow::Initialize(title);
    createDXGISwapChain(device->adapter->instance->pDXGIFactory, device->pD3D12Queue);
    return succeed;
}

bool LittleGFXWindow::Destroy()
{
    auto succeed = LittleWindow::Destroy();
    SAFE_RELEASE(pSwapChain);
    return succeed;
}

void LittleGFXWindow::createDXGISwapChain(struct IDXGIFactory6* pDXGIFactory, struct ID3D12CommandQueue* present_queue)
{
    DXGI_SWAP_CHAIN_DESC1 chain_desc1 = { 0 };
    chain_desc1.Width = width;
    chain_desc1.Height = height;
    // https://docs.microsoft.com/en-us/windows/win32/api/dxgi1_2/ns-dxgi1_2-dxgi_swap_chain_desc1
    chain_desc1.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    chain_desc1.Stereo = false;
    chain_desc1.SampleDesc.Count = 1; // If multisampling is needed, we'll resolve it later
    chain_desc1.SampleDesc.Quality = 0;
    chain_desc1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    chain_desc1.BufferCount = vsyncEnabled ? 3 : 2;
    chain_desc1.Scaling = DXGI_SCALING_STRETCH;
    chain_desc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // for better performance.
    chain_desc1.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    chain_desc1.Flags = 0;

    BOOL allowTearing = FALSE;
    chain_desc1.Flags |= allowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
    swapchainFlags |= (!vsyncEnabled && allowTearing) ? DXGI_PRESENT_ALLOW_TEARING : 0;

    IDXGISwapChain1* swapchain;
    auto bCreated = SUCCEEDED(pDXGIFactory->CreateSwapChainForHwnd(present_queue,
        hWnd, &chain_desc1, NULL, NULL, &swapchain));
    assert(bCreated && "Failed to Try to Create SwapChain!");

    auto bAssociation = SUCCEEDED(
        pDXGIFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));
    assert(bAssociation && "Failed to Try to Associate SwapChain With Window!");

    auto bQueryChain3 = SUCCEEDED(swapchain->QueryInterface(IID_PPV_ARGS(&pSwapChain)));
    assert(bQueryChain3 && "Failed to Query IDXGISwapChain3 from Created SwapChain!");

    SAFE_RELEASE(swapchain);
}