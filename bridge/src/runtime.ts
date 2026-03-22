import { Platform } from "react-native";
import NativeCanvasOSEngine from "./NativeCanvasOSEngine";

export function getCanvasOSEngineModule() {
  if (Platform.OS !== "android") {
    return null;
  }
  return NativeCanvasOSEngine ?? null;
}

export function hasCanvasOSEngineModule() {
  return getCanvasOSEngineModule() !== null;
}
