// Copyright (c) Microsoft Corporation. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may
// not use these files except in compliance with the License. You may obtain
// a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.

#pragma once

#include <Canvas.abi.h>

#include <RegisteredEvent.h>

namespace ABI { namespace Microsoft { namespace Graphics { namespace Canvas
{
    using namespace ABI::Windows::Foundation;
    using namespace ABI::Windows::UI::Core;
    using namespace ABI::Windows::UI::Xaml;
    using namespace ABI::Windows::UI::Xaml::Controls;
    using namespace ABI::Windows::UI::Xaml::Media;
    using namespace ABI::Windows::Graphics::Display;

    class CanvasDrawEventArgsFactory : public ActivationFactory<ICanvasDrawEventArgsFactory>
    {
        InspectableClassStatic(RuntimeClass_Microsoft_Graphics_Canvas_CanvasDrawEventArgs, BaseTrust);

    public:
        IFACEMETHOD(Create)(
            ICanvasDrawingSession* drawingSession,
            ICanvasDrawEventArgs** drawEventArgs) override;
    };

    class CanvasDrawEventArgs : public RuntimeClass<
        ICanvasDrawEventArgs>
    {
        InspectableClass(RuntimeClass_Microsoft_Graphics_Canvas_CanvasDrawEventArgs, BaseTrust);

        ClosablePtr<ICanvasDrawingSession> m_drawingSession;

     public:
         CanvasDrawEventArgs(ICanvasDrawingSession* drawingSession);

         IFACEMETHODIMP get_DrawingSession(ICanvasDrawingSession** value);
    };

    typedef ITypedEventHandler<CanvasControl*, IInspectable*> CreateResourcesEventHandlerType;
    typedef ITypedEventHandler<CanvasControl*, CanvasDrawEventArgs*> DrawEventHandlerType;

    class ICanvasControlAdapter
    {
    public:
        virtual std::pair<ComPtr<IInspectable>, ComPtr<IUserControl>> CreateUserControl(IInspectable* canvasControl) = 0;
        virtual ComPtr<ICanvasDevice> CreateCanvasDevice() = 0;
        virtual RegisteredEvent AddCompositionRenderingCallback(IEventHandler<IInspectable*>*) = 0;
        virtual RegisteredEvent AddSurfaceContentsLostCallback(IEventHandler<IInspectable*>*) = 0;
        virtual ComPtr<CanvasImageSource> CreateCanvasImageSource(ICanvasDevice* device, int width, int height) = 0;
        virtual ComPtr<IImage> CreateImageControl() = 0;
        virtual float GetLogicalDpi() = 0;

        typedef ITypedEventHandler<DisplayInformation*, IInspectable*> DpiChangedHandler;
        virtual RegisteredEvent AddDpiChangedCallback(DpiChangedHandler* handler) = 0;

        virtual ComPtr<IWindow> GetCurrentWindow() = 0;

        template<typename T, typename METHOD>
        RegisteredEvent AddCompositionRenderingCallback(T* obj, METHOD method)
        {
            auto callback = Callback<IEventHandler<IInspectable*>>(obj, method);
            CheckMakeResult(callback);
            return AddCompositionRenderingCallback(callback.Get());
        }

        template<typename T, typename METHOD>
        RegisteredEvent AddSurfaceContentsLostCallback(T* obj, METHOD method)
        {
            auto callback = Callback<IEventHandler<IInspectable*>>(obj, method);
            CheckMakeResult(callback);
            return AddSurfaceContentsLostCallback(callback.Get());
        }

        template<typename T, typename METHOD>
        RegisteredEvent AddDpiChangedCallback(T* obj, METHOD method)
        {
            auto callback = Callback<DpiChangedHandler>(obj, method);
            CheckMakeResult(callback);
            return AddDpiChangedCallback(callback.Get());
        }
    };

    class CanvasControl : public RuntimeClass<
        RuntimeClassFlags<WinRtClassicComMix>,
        ICanvasControl,
        ICanvasResourceCreator,
        ABI::Windows::UI::Xaml::IFrameworkElementOverrides,
        ComposableBase<ABI::Windows::UI::Xaml::Controls::IUserControl>>
    {
        InspectableClass(RuntimeClass_Microsoft_Graphics_Canvas_CanvasControl, BaseTrust);

        std::mutex m_drawLock;

        std::shared_ptr<ICanvasControlAdapter> m_adapter;

        // The current window is thread local.  We grab this on construction
        // since this will happen on the correct thread.  From then on we use
        // this stored value since we can't always be sure that we'll always be
        // called from that window's thread.
        ComPtr<IWindow> m_window;

        EventSource<CreateResourcesEventHandlerType> m_createResourcesEventList;
        EventSource<DrawEventHandlerType> m_drawEventList;

        RegisteredEvent m_surfaceContentsLostEventRegistration;
        RegisteredEvent m_windowVisibilityChangedEventRegistration;
        RegisteredEvent m_renderingEventRegistration;
        RegisteredEvent m_dpiChangedEventRegistration;

        ComPtr<ICanvasDevice> m_canvasDevice;
        ComPtr<IImage> m_imageControl;
        ComPtr<ICanvasImageSource> m_canvasImageSource;
        bool m_needToHookCompositionRendering;
        bool m_imageSourceNeedsReset;
        bool m_isLoaded;

        int m_currentWidth;
        int m_currentHeight;
        
    public:
        CanvasControl(
            std::shared_ptr<ICanvasControlAdapter> adapter);

        ~CanvasControl();

        //
        // ICanvasControl
        //

        IFACEMETHODIMP add_CreateResources(
            CreateResourcesEventHandlerType* value,
            EventRegistrationToken *token);

        IFACEMETHODIMP remove_CreateResources(
            EventRegistrationToken token);

        IFACEMETHODIMP add_Draw(
            DrawEventHandlerType* value,
            EventRegistrationToken* token);

        IFACEMETHODIMP remove_Draw(
            EventRegistrationToken token);

        //
        // ICanvasResourceCreator
        //

        IFACEMETHODIMP get_Device(ICanvasDevice** value);

        IFACEMETHODIMP Invalidate();

        //
        // IFrameworkElementOverrides
        //

        IFACEMETHODIMP MeasureOverride(
            ABI::Windows::Foundation::Size availableSize, 
            ABI::Windows::Foundation::Size* returnValue);

        IFACEMETHODIMP ArrangeOverride(
            ABI::Windows::Foundation::Size finalSize, 
            ABI::Windows::Foundation::Size* returnValue);

        IFACEMETHODIMP OnApplyTemplate();

        HRESULT OnLoaded(IInspectable* sender, IRoutedEventArgs* args);
        HRESULT OnSizeChanged(IInspectable* sender, ISizeChangedEventArgs* args);

    private:
        void CreateBaseClass();
        void CreateImageControl();
        void RegisterEventHandlers();

        template<typename T, typename DELEGATE, typename HANDLER>
        void RegisterEventHandlerOnSelf(
            ComPtr<T> const& self, 
            HRESULT (STDMETHODCALLTYPE T::* addMethod)(DELEGATE*, EventRegistrationToken*), 
            HANDLER handler);

        bool IsWindowVisible();

        void PreDraw();
        void EnsureSizeDependentResources();
        void CallDrawHandlers();

        enum class InvalidateReason
        {
            Default,
            SurfaceContentsLost
        };

        void InvalidateImpl(InvalidateReason reason = InvalidateReason::Default);

        void HookCompositionRenderingIfNecessary();

        HRESULT OnCompositionRendering(IInspectable* sender, IInspectable* args);        
        HRESULT OnDpiChanged(IDisplayInformation* sender, IInspectable* args);
        HRESULT OnSurfaceContentsLost(IInspectable* sender, IInspectable* args);
        HRESULT OnWindowVisibilityChanged(IInspectable* sender, IVisibilityChangedEventArgs* args);
    };

}}}}
