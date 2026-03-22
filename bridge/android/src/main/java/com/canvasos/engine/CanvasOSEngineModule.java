package com.canvasos.engine;

import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.ReactContextBaseJavaModule;
import com.facebook.react.module.annotations.ReactModule;
import com.facebook.react.turbomodule.core.interfaces.TurboModule;

@ReactModule(name = CanvasOSEngineModule.NAME)
public class CanvasOSEngineModule extends ReactContextBaseJavaModule implements TurboModule {
    public static final String NAME = "CanvasOSEngine";

    public CanvasOSEngineModule(ReactApplicationContext reactContext) {
        super(reactContext);
    }

    @Override
    public String getName() {
        return NAME;
    }

    static {
        System.loadLibrary("canvasos_engine");
    }
}
