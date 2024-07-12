// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "log.h"
#include "ffx_api.h"
#include "ffx_upscale.h"
#include "dx12/ffx_api_dx12.h"
#include "ffx_fsr2.h"
#include "dx12/ffx_fsr2_dx12.h"

#if _DEBUG
#pragma comment(lib, "ffx_fsr2_212_api_x64d.lib")
#pragma comment(lib, "ffx_fsr2_212_api_dx12_x64d.lib")
#else
#pragma comment(lib, "ffx_fsr2_212_api_x64.lib")
#pragma comment(lib, "ffx_fsr2_212_api_dx12_x64.lib")
#endif // _DEBUG


static HMODULE _amdDll = nullptr;
static PfnFfxCreateContext _createContext = nullptr;
static PfnFfxDestroyContext _destroyContext = nullptr;
static PfnFfxConfigure _configure = nullptr;
static PfnFfxQuery _query = nullptr;
static PfnFfxDispatch _dispatch = nullptr;

static Fsr212::FfxFsr2Context _context = {};
static Fsr212::FfxFsr2ContextDescription _contextDesc = {};
static void* scratchBuffer;

FFX_API_ENTRY ffxReturnCode_t ffxCreateContext(ffxContext* context, ffxCreateContextDescHeader* desc, const ffxAllocationCallbacks* memCb)
{
    if (_createContext == nullptr)
        return FFX_API_RETURN_ERROR;

    log("ffxCreateContext");

    ffxCreateContextDescHeader* header = desc;

    if (desc->type == FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE)
    {
        log("ffxCreateContext UPSCALER_CONTEXT_HEADER, using fsr21");

        Fsr212::FfxErrorCode result;
        Fsr212::FfxErrorCode createResult = Fsr212::FFX_ERROR_INVALID_ARGUMENT;

        const size_t scratchBufferSize = Fsr212::ffxFsr2GetScratchMemorySizeDX12_212();

        while (header != nullptr)
        {
            if (header->type == FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE)
            {
                log("ffxCreateContext header->type: FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE");

                scratchBuffer = calloc(scratchBufferSize, 1);

                auto cc = (ffxCreateContextDescUpscale*)header;

                _contextDesc.flags = cc->flags;
                _contextDesc.displaySize.width = cc->maxUpscaleSize.width;
                _contextDesc.displaySize.height = cc->maxUpscaleSize.height;
                _contextDesc.maxRenderSize.width = cc->maxRenderSize.width;
                _contextDesc.maxRenderSize.height = cc->maxRenderSize.height;
            }
            else if (header->type == FFX_API_DESC_TYPE_OVERRIDE_VERSION)
            {
                log("ffxCreateContext header->type: FFX_API_DESC_TYPE_OVERRIDE_VERSION");
            }
            else if (header->type == FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12)
            {
                log("ffxCreateContext header->type: FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12");
                auto dh = (ffxCreateBackendDX12Desc*)header;

                result = Fsr212::ffxFsr2GetInterfaceDX12_212(&_contextDesc.callbacks, dh->device, scratchBuffer, scratchBufferSize);

                if (result != Fsr212::FFX_OK)
                {
                    free(scratchBuffer);
                    return FFX_API_RETURN_ERROR;
                }

                _contextDesc.device = Fsr212::ffxGetDeviceDX12_212(dh->device);
            }
            else
            {
                log("ffxCreateContext header->type: " + std::to_string((uint64_t)header->type));
            }

            header = header->pNext;
        }

        createResult = Fsr212::ffxFsr2ContextCreate212(&_context, &_contextDesc);

        if (createResult == Fsr212::FFX_OK)
        {
            *context = (ffxContext*)(void*)&_context;
            return FFX_API_RETURN_OK;
        }
    }

    auto result = _createContext(context, desc, memCb);

    log("ffxCreateContext result: " + std::to_string((uint32_t)result));

    return result;
}

FFX_API_ENTRY ffxReturnCode_t ffxDestroyContext(ffxContext* context, const ffxAllocationCallbacks* memCb)
{
    log("ffxDestroyContext");

    if (*context == &_context)
    {
        auto errorCode = Fsr212::ffxFsr2ContextDestroy212(&_context);
        free(_contextDesc.callbacks.scratchBuffer);

        return FFX_API_RETURN_OK;
    }

    auto result = _destroyContext(context, memCb);
    log("ffxDestroyContext result: " + std::to_string((uint32_t)result));

    return result;
}

FFX_API_ENTRY ffxReturnCode_t ffxConfigure(ffxContext* context, const ffxConfigureDescHeader* desc)
{
    log("ffxConfigure");

    auto result = _configure(context, desc);

    log("ffxConfigure result: " + std::to_string((uint32_t)result));

    return result;
}

FFX_API_ENTRY ffxReturnCode_t ffxQuery(ffxContext* context, ffxQueryDescHeader* desc)
{
    log("ffxQuery");

    auto result = _query(context, desc);

    log("ffxQuery result: " + std::to_string((uint32_t)result));

    return result;
}

FFX_API_ENTRY ffxReturnCode_t ffxDispatch(ffxContext* context, const ffxDispatchDescHeader* desc)
{
    log("ffxDispatch");

    if (desc->type == FFX_API_DISPATCH_DESC_TYPE_UPSCALE)
    {
        log("ffxDispatch desc->type: FFX_API_DISPATCH_DESC_TYPE_UPSCALE");
        auto ud = (ffxDispatchDescUpscale*)desc;

        log("ffxDispatch ud->cameraFar: " + std::to_string(ud->cameraFar));
        log("ffxDispatch ud->cameraFovAngleVertical: " + std::to_string(ud->cameraFovAngleVertical));
        log("ffxDispatch ud->cameraNear: " + std::to_string(ud->cameraNear));

        if (ud->color.resource != nullptr)
            log("ffxDispatch ud->color: not null");
        else
            log("ffxDispatch ud->color: null");

        if (ud->commandList != nullptr)
            log("ffxDispatch ud->commandList: not null");
        else
            log("ffxDispatch ud->commandList: null");

        if (ud->depth.resource != nullptr)
            log("ffxDispatch ud->depth: not null");
        else
            log("ffxDispatch ud->depth: null");

        if (ud->enableSharpening)
            log("ffxDispatch ud->enableSharpening: true");
        else
            log("ffxDispatch ud->enableSharpening: false");

        if (ud->exposure.resource != nullptr)
            log("ffxDispatch ud->exposure: not null");
        else
            log("ffxDispatch ud->exposure: null");

        log("ffxDispatch ud->flags: " + std::to_string(ud->flags));
        log("ffxDispatch ud->frameTimeDelta: " + std::to_string(ud->frameTimeDelta));
        log("ffxDispatch ud->jitterOffset: {" + std::to_string(ud->jitterOffset.x) + ", " + std::to_string(ud->jitterOffset.y) + "}");

        if (ud->motionVectors.resource != nullptr)
            log("ffxDispatch ud->motionVectors: not null");
        else
            log("ffxDispatch ud->motionVectors: null");

        log("ffxDispatch ud->motionVectorScale: {" + std::to_string(ud->motionVectorScale.x) + ", " + std::to_string(ud->motionVectorScale.y) + "}");

        if ((ud->output.resource != nullptr))
            log("ffxDispatch ud->output: not null");
        else
            log("ffxDispatch ud->output: null");

        log("ffxDispatch ud->preExposure: " + std::to_string(ud->preExposure));

        if ((ud->reactive.resource != nullptr))
            log("ffxDispatch ud->reactive: not null");
        else
            log("ffxDispatch ud->reactive: null");

        log("ffxDispatch ud->renderSize: {" + std::to_string(ud->renderSize.width) + ", " + std::to_string(ud->renderSize.height) + "}");
        log("ffxDispatch ud->reset: " + std::to_string(ud->reset));
        log("ffxDispatch ud->sharpness: " + std::to_string(ud->sharpness));

        if (ud->transparencyAndComposition.resource != nullptr)
            log("ffxDispatch ud->transparencyAndComposition: not null");
        else
            log("ffxDispatch ud->transparencyAndComposition: null");

        log("ffxDispatch ud->upscaleSize: {" + std::to_string(ud->upscaleSize.width) + ", " + std::to_string(ud->upscaleSize.height) + "}");
        log("ffxDispatch ud->viewSpaceToMetersFactor: " + std::to_string(ud->viewSpaceToMetersFactor));

        if (*context == &_context)
        {
            Fsr212::FfxFsr2DispatchDescription params{};

            params.cameraFar = ud->cameraFar;
            params.cameraFovAngleVertical = ud->cameraFovAngleVertical;
            params.cameraNear = ud->cameraNear;

            if (((FfxApiResource)ud->color).resource)
                params.color = Fsr212::ffxGetResourceDX12_212(&_context, (ID3D12Resource*)((FfxApiResource)ud->color).resource, NULL, Fsr212::FFX_RESOURCE_STATE_COMPUTE_READ);

            params.commandList = ud->commandList;

            if (((FfxApiResource)ud->depth).resource)
                params.depth = Fsr212::ffxGetResourceDX12_212(&_context, (ID3D12Resource*)((FfxApiResource)ud->depth).resource, NULL, Fsr212::FFX_RESOURCE_STATE_COMPUTE_READ);

            params.enableSharpening = ud->enableSharpening;

            if (((FfxApiResource)ud->exposure).resource)
                params.exposure = Fsr212::ffxGetResourceDX12_212(&_context, (ID3D12Resource*)((FfxApiResource)ud->exposure).resource, NULL, Fsr212::FFX_RESOURCE_STATE_COMPUTE_READ);

            params.frameTimeDelta = ud->frameTimeDelta;
            params.jitterOffset.x = ud->jitterOffset.x;
            params.jitterOffset.y = ud->jitterOffset.y;

            if (((FfxApiResource)ud->motionVectors).resource)
                params.motionVectors = Fsr212::ffxGetResourceDX12_212(&_context, (ID3D12Resource*)((FfxApiResource)ud->motionVectors).resource, NULL, Fsr212::FFX_RESOURCE_STATE_COMPUTE_READ);

            params.motionVectorScale.x = ud->motionVectorScale.x;
            params.motionVectorScale.y = ud->motionVectorScale.y;

            if (((FfxApiResource)ud->output).resource)
                params.output = Fsr212::ffxGetResourceDX12_212(&_context, (ID3D12Resource*)((FfxApiResource)ud->output).resource, NULL, Fsr212::FFX_RESOURCE_STATE_UNORDERED_ACCESS);

            params.preExposure = ud->preExposure;

            if (((FfxApiResource)ud->reactive).resource)
                params.reactive = Fsr212::ffxGetResourceDX12_212(&_context, (ID3D12Resource*)((FfxApiResource)ud->reactive).resource, NULL, Fsr212::FFX_RESOURCE_STATE_COMPUTE_READ);

            params.renderSize.width = ud->renderSize.width;
            params.renderSize.height = ud->renderSize.height;
            params.reset = ud->reset;
            params.sharpness = ud->sharpness;

            if (((FfxApiResource)ud->transparencyAndComposition).resource)
                params.transparencyAndComposition = Fsr212::ffxGetResourceDX12_212(&_context, (ID3D12Resource*)((FfxApiResource)ud->transparencyAndComposition).resource, NULL, Fsr212::FFX_RESOURCE_STATE_COMPUTE_READ);


            auto result = Fsr212::ffxFsr2ContextDispatch212(&_context, &params);

            if (result != Fsr212::FFX_OK)
                return FFX_API_RETURN_ERROR;

            return FFX_API_RETURN_OK;
        }
    }
    else if (desc->type == FFX_API_DISPATCH_DESC_TYPE_UPSCALE_GENERATEREACTIVEMASK)
    {
        log("ffxDispatch desc->type: FFX_API_DISPATCH_DESC_TYPE_UPSCALE_GENERATEREACTIVEMASK");
    }
    else
    {
        log("ffxDispatch desc->type: " + std::to_string((uint64_t)desc->type));
    }

    auto result = _dispatch(context, desc);

    log("ffxDispatch result: " + std::to_string((uint32_t)result));

    return result;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hModule);

            prepareLogging("fsr31proxy.log");

            _amdDll = LoadLibrary(L"amd_fidelityfx_dx12.o.dll");

            if (_amdDll != nullptr)
            {
                _createContext = (PfnFfxCreateContext)GetProcAddress(_amdDll, "ffxCreateContext");
                _destroyContext = (PfnFfxDestroyContext)GetProcAddress(_amdDll, "ffxDestroyContext");
                _configure = (PfnFfxConfigure)GetProcAddress(_amdDll, "ffxConfigure");
                _query = (PfnFfxQuery)GetProcAddress(_amdDll, "ffxQuery");
                _dispatch = (PfnFfxDispatch)GetProcAddress(_amdDll, "ffxDispatch");
            }

            break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            closeLogging();
            FreeLibrary(_amdDll);
            break;
    }

    return TRUE;
}

