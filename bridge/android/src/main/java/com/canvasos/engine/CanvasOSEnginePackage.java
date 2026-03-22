package com.canvasos.engine;

import com.facebook.react.TurboReactPackage;
import com.facebook.react.bridge.NativeModule;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.module.model.ReactModuleInfo;
import com.facebook.react.module.model.ReactModuleInfoProvider;
import java.util.HashMap;
import java.util.Map;

public class CanvasOSEnginePackage extends TurboReactPackage {
    @Override
    public NativeModule getModule(String name, ReactApplicationContext reactContext) {
        if (CanvasOSEngineModule.NAME.equals(name)) {
            return new CanvasOSEngineModule(reactContext);
        }
        return null;
    }

    @Override
    public ReactModuleInfoProvider getReactModuleInfoProvider() {
        return () -> {
            Map<String, ReactModuleInfo> modules = new HashMap<>();
            modules.put(
                CanvasOSEngineModule.NAME,
                new ReactModuleInfo(
                    CanvasOSEngineModule.NAME,
                    CanvasOSEngineModule.NAME,
                    false,
                    false,
                    false,
                    true
                )
            );
            return modules;
        };
    }
}
