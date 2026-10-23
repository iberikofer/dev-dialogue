'use client';

import React, { useState, useEffect, useRef, useCallback } from 'react';
import { useRouter } from 'next/navigation';
import {
  Send,
  Bot,
  User,
  CheckCircle,
  Sparkles,
  AlertCircle,
  Clock,
  Volume2,
  VolumeX,
  Smile,
  LayoutGrid,
  Maximize2,
  Columns2,
  SlidersHorizontal,
  X,
  Play,
  RotateCcw,
  Gauge,
  Activity,
  Palette,
} from 'lucide-react';
import { InterviewMessage } from '@dev-dialogue/api-contracts';
import { api } from '@/lib/api-client';
import { useTranslation } from '@/lib/useTranslation';
import { useAuthStore } from '@/store/authStore';

interface ChatRoomProps {
  interviewId: string;
  initialDirection?: string;
  initialLevel?: string;
  initialLanguage?: string;
}

const MASCOT_BASE = '/mascot/';
const SOUNDS_BASE = '/mascot/Sounds/';

interface TexturePresetDef {
  id: string;
  nameUk: string;
  nameEn: string;
  icon: string;
  descUk: string;
  descEn: string;
  folder: string | null;
  metalness: number;
  roughness: number;
  skinColor: number;
  emissive?: number;
  emissiveIntensity?: number;
}

const TEXTURE_PRESETS: TexturePresetDef[] = [
  {
    id: 'realistic',
    nameUk: 'Реалістична',
    nameEn: 'Realistic',
    icon: '👤',
    descUk: "Анатомічна текстура шкіри з порами, рум'янцем та карими очима",
    descEn: 'Natural anatomical human skin, flush, pores & hazel eyes',
    folder: '/mascot/textures/realistic/',
    metalness: 0.0,
    roughness: 1.0,
    skinColor: 0xffffff,
    emissive: 0x000000,
    emissiveIntensity: 0.0,
  },
  {
    id: 'cyber',
    nameUk: 'Кіберпанк',
    nameEn: 'Cyberpunk',
    icon: '🤖',
    descUk: 'Синтетична титанова броня з неоновими бірюзовими мікросхемами',
    descEn: 'Titanium shell with glowing cyan circuits & optical sensor',
    folder: '/mascot/textures/cyber/',
    metalness: 0.65,
    roughness: 0.60,
    skinColor: 0xffffff,
    emissive: 0x00c8e0,
    emissiveIntensity: 0.85,
  },
  {
    id: 'alien',
    nameUk: 'Алієн',
    nameEn: 'Alien',
    icon: '👽',
    descUk: 'Монотонна фіолетова шкіра з біолюмінесцентними точками',
    descEn: 'Monotonic violet alien skin with bioluminescent markings',
    folder: '/mascot/textures/alien/',
    metalness: 0.08,
    roughness: 0.85,
    skinColor: 0xffffff,
    emissive: 0xd08020,
    emissiveIntensity: 0.65,
  },
  {
    id: 'noir',
    nameUk: 'Нуар',
    nameEn: 'Noir',
    icon: '🎬',
    descUk: 'Висококонтрастний монохромний нуар зі стрімкими тінями',
    descEn: 'High-contrast monochromatic graphic novel style',
    folder: '/mascot/textures/noir/',
    metalness: 0.0,
    roughness: 1.0,
    skinColor: 0xffffff,
    emissive: 0x000000,
    emissiveIntensity: 0.0,
  },
  {
    id: 'stylized',
    nameUk: 'Аніме',
    nameEn: 'Anime',
    icon: '🎨',
    descUk: "Порцелянова шкіра, ніжний рум'янець та виразні смарагдові очі",
    descEn: 'Porcelain cel-shaded skin & sparkling emerald eyes',
    folder: '/mascot/textures/stylized/',
    metalness: 0.0,
    roughness: 0.90,
    skinColor: 0xffffff,
    emissive: 0x000000,
    emissiveIntensity: 0.0,
  },
  {
    id: 'clay',
    nameUk: 'Глина',
    nameEn: 'Studio Clay',
    icon: '🏛️',
    descUk: 'Мінімалістична матова скульптурна глина без текстур',
    descEn: 'Clean untextured matte sculpture finish',
    folder: null,
    metalness: 0.0,
    roughness: 0.88,
    skinColor: 0xbaa898,
  },
];

// eslint-disable-next-line @typescript-eslint/no-explicit-any
function applyPresetToMaterials(THREE: any, mats: Record<string, any>, preset: TexturePresetDef, textureCache: Map<string, any>, texLoader: any) {
  if (!THREE || !mats || !texLoader) return;
  const loadCached = (url: string, isSRGB = true) => {
    if (!textureCache.has(url)) {
      const tex = texLoader.load(url);
      tex.flipY = false;
      tex.colorSpace = isSRGB ? THREE.SRGBColorSpace : THREE.NoColorSpace;
      textureCache.set(url, tex);
    }
    return textureCache.get(url);
  };

  const skinMat = mats['basemeshmaterial'];
  const irisMat = mats['Mat_Iris'];
  const scleraMat = mats['Mat_Sclera'];
  const pupilMat = mats['Mat_Pupil'];
  const browMat = mats['Mat_Eyebrow'];
  const mouthMat = mats['MouthCavity'];
  const mouthDeepMat = mats['MouthCavityDeep'];
  const teethMat = mats['Mat_Teeth'];
  const gumsMat = mats['Mat_Gums'];
  const tongueMat = mats['Mat_Tongue'];

  if (!preset.folder) {
    if (skinMat) {
      skinMat.map = null;
      skinMat.roughnessMap = null;
      skinMat.color.setHex(preset.skinColor);
      skinMat.roughness = preset.roughness;
      skinMat.metalness = preset.metalness;
      skinMat.needsUpdate = true;
    }
    if (irisMat) {
      irisMat.map = null;
      irisMat.color.setHex(0xbaa898);
      if (irisMat.emissive) irisMat.emissive.setHex(0x000000);
      irisMat.roughness = 0.88;
      irisMat.metalness = 0.0;
      irisMat.needsUpdate = true;
    }
    if (scleraMat) {
      scleraMat.map = null;
      scleraMat.color.setHex(0xbaa898);
      scleraMat.roughness = 0.88;
      scleraMat.needsUpdate = true;
    }
    if (pupilMat) {
      pupilMat.color.setHex(0x33302c);
      pupilMat.needsUpdate = true;
    }
    if (browMat) {
      browMat.map = null;
      browMat.color.setHex(0xa89688);
      browMat.roughness = 0.88;
      browMat.needsUpdate = true;
    }
    if (mouthMat) {
      mouthMat.color.setHex(0x605850);
      mouthMat.needsUpdate = true;
    }
    if (mouthDeepMat) {
      mouthDeepMat.color.setHex(0x403830);
      mouthDeepMat.needsUpdate = true;
    }
    if (teethMat) {
      teethMat.color.setHex(0xd5c8bc);
      teethMat.roughness = 0.88;
      teethMat.metalness = 0.0;
      teethMat.needsUpdate = true;
    }
    if (gumsMat) {
      gumsMat.color.setHex(0xb09080);
      gumsMat.roughness = 0.88;
      gumsMat.metalness = 0.0;
      gumsMat.needsUpdate = true;
    }
    if (tongueMat) {
      tongueMat.color.setHex(0xa88878);
      tongueMat.roughness = 0.88;
      tongueMat.metalness = 0.0;
      tongueMat.needsUpdate = true;
    }
  } else {
    const skinDiff = loadCached(preset.folder + 'skin_diffuse.png', true);
    const skinRough = loadCached(preset.folder + 'skin_roughness.png', false);
    const irisDiff = loadCached(preset.folder + 'iris_diffuse.png', true);
    const scleraDiff = loadCached(preset.folder + 'sclera_diffuse.png', true);
    const browDiff = loadCached(preset.folder + 'eyebrow_diffuse.png', true);

    if (skinMat) {
      skinMat.map = skinDiff;
      skinMat.roughnessMap = skinRough;
      skinMat.color.setHex(preset.skinColor);
      skinMat.metalness = preset.metalness;
      skinMat.roughness = preset.roughness;
      skinMat.needsUpdate = true;
    }
    if (irisMat) {
      irisMat.map = irisDiff;
      irisMat.color.setHex(0xffffff);
      if (irisMat.emissive) irisMat.emissive.setHex(preset.emissive ?? 0x000000);
      if (irisMat.emissiveIntensity !== undefined) irisMat.emissiveIntensity = preset.emissiveIntensity ?? 0.0;
      irisMat.roughness = 0.10;
      irisMat.needsUpdate = true;
    }
    if (scleraMat) {
      scleraMat.map = scleraDiff;
      scleraMat.color.setHex(0xffffff);
      scleraMat.roughness = 0.15;
      scleraMat.needsUpdate = true;
    }
    if (pupilMat) {
      pupilMat.color.setHex(preset.id === 'cyber' ? 0x051a22 : preset.id === 'noir' ? 0x000000 : 0x030303);
      pupilMat.needsUpdate = true;
    }
    if (browMat) {
      browMat.map = browDiff;
      browMat.color.setHex(0xffffff);
      browMat.roughness = 0.75;
      browMat.needsUpdate = true;
    }
    if (mouthMat) {
      mouthMat.color.setHex(preset.id === 'cyber' ? 0x223344 : preset.id === 'alien' ? 0x281242 : preset.id === 'noir' ? 0x202020 : 0x3d0b0b);
      mouthMat.needsUpdate = true;
    }
    if (mouthDeepMat) {
      mouthDeepMat.color.setHex(preset.id === 'cyber' ? 0x111c24 : preset.id === 'alien' ? 0x140822 : preset.id === 'noir' ? 0x080808 : 0x150303);
      mouthDeepMat.needsUpdate = true;
    }
    if (teethMat) {
      if (preset.id === 'cyber') {
        teethMat.color.setHex(0xd8e8f2);
        teethMat.roughness = 0.12;
        teethMat.metalness = 0.75;
      } else if (preset.id === 'alien') {
        teethMat.color.setHex(0xc0e0f8);
        teethMat.roughness = 0.18;
        teethMat.metalness = 0.10;
      } else if (preset.id === 'noir') {
        teethMat.color.setHex(0xffffff);
        teethMat.roughness = 0.20;
        teethMat.metalness = 0.0;
      } else if (preset.id === 'stylized') {
        teethMat.color.setHex(0xfffff8);
        teethMat.roughness = 0.18;
        teethMat.metalness = 0.0;
      } else {
        teethMat.color.setHex(0xf2ede2);
        teethMat.roughness = 0.22;
        teethMat.metalness = 0.0;
      }
      teethMat.needsUpdate = true;
    }
    if (gumsMat) {
      if (preset.id === 'cyber') {
        gumsMat.color.setHex(0x1a2e38);
        gumsMat.roughness = 0.45;
        gumsMat.metalness = 0.3;
      } else if (preset.id === 'alien') {
        gumsMat.color.setHex(0x481b66);
        gumsMat.roughness = 0.40;
        gumsMat.metalness = 0.0;
      } else if (preset.id === 'noir') {
        gumsMat.color.setHex(0x444444);
        gumsMat.roughness = 0.45;
        gumsMat.metalness = 0.0;
      } else if (preset.id === 'stylized') {
        gumsMat.color.setHex(0xe8687a);
        gumsMat.roughness = 0.35;
        gumsMat.metalness = 0.0;
      } else {
        gumsMat.color.setHex(0xb24552);
        gumsMat.roughness = 0.38;
        gumsMat.metalness = 0.0;
      }
      gumsMat.needsUpdate = true;
    }
    if (tongueMat) {
      if (preset.id === 'cyber') {
        tongueMat.color.setHex(0x162c36);
        tongueMat.roughness = 0.40;
        tongueMat.metalness = 0.2;
      } else if (preset.id === 'alien') {
        tongueMat.color.setHex(0x602078);
        tongueMat.roughness = 0.40;
        tongueMat.metalness = 0.0;
      } else if (preset.id === 'noir') {
        tongueMat.color.setHex(0x383838);
        tongueMat.roughness = 0.45;
        tongueMat.metalness = 0.0;
      } else if (preset.id === 'stylized') {
        tongueMat.color.setHex(0xeb6478);
        tongueMat.roughness = 0.40;
        tongueMat.metalness = 0.0;
      } else {
        tongueMat.color.setHex(0xad4148);
        tongueMat.roughness = 0.45;
        tongueMat.metalness = 0.0;
      }
      tongueMat.needsUpdate = true;
    }
  }
}

const VISEME_MAP: Record<string, Record<string, number>> = {
  а: { jawOpen: 0.48, mouthWide: 0.22, mouthSmileLeft: 0.08, mouthSmileRight: 0.08, cheekSquintLeft: 0.12, cheekSquintRight: 0.12, mouthUpperUpLeft: 0.1, mouthUpperUpRight: 0.1, browInnerUp: 0.08, tongueUp: 0 },
  a: { jawOpen: 0.48, mouthWide: 0.22, mouthSmileLeft: 0.08, mouthSmileRight: 0.08, cheekSquintLeft: 0.12, cheekSquintRight: 0.12, mouthUpperUpLeft: 0.1, mouthUpperUpRight: 0.1, browInnerUp: 0.08, tongueUp: 0 },
  я: { jawOpen: 0.44, mouthWide: 0.3, mouthSmileLeft: 0.12, mouthSmileRight: 0.12, cheekSquintLeft: 0.14, cheekSquintRight: 0.14, mouthUpperUpLeft: 0.1, mouthUpperUpRight: 0.1, browInnerUp: 0.08, tongueUp: 0.2 },
  о: { jawOpen: 0.38, mouthFunnel: 0.46, mouthPucker: 0.22, cheekSquintLeft: 0.08, cheekSquintRight: 0.08, browInnerUp: 0.06, tongueUp: 0 },
  o: { jawOpen: 0.38, mouthFunnel: 0.46, mouthPucker: 0.22, cheekSquintLeft: 0.08, cheekSquintRight: 0.08, browInnerUp: 0.06, tongueUp: 0 },
  у: { jawOpen: 0.2, mouthPucker: 0.55, mouthFunnel: 0.26, browInnerUp: 0.05, tongueUp: 0 },
  u: { jawOpen: 0.2, mouthPucker: 0.55, mouthFunnel: 0.26, browInnerUp: 0.05, tongueUp: 0 },
  ю: { jawOpen: 0.2, mouthPucker: 0.5, mouthFunnel: 0.24, cheekSquintLeft: 0.06, cheekSquintRight: 0.06, tongueUp: 0.25 },
  w: { jawOpen: 0.18, mouthPucker: 0.58, mouthFunnel: 0.25, tongueUp: 0 },
  і: { jawOpen: 0.18, mouthWide: 0.5, mouthSmileLeft: 0.2, mouthSmileRight: 0.2, cheekSquintLeft: 0.16, cheekSquintRight: 0.16, mouthUpperUpLeft: 0.12, mouthUpperUpRight: 0.12, tongueUp: 0.2 },
  i: { jawOpen: 0.18, mouthWide: 0.5, mouthSmileLeft: 0.2, mouthSmileRight: 0.2, cheekSquintLeft: 0.16, cheekSquintRight: 0.16, mouthUpperUpLeft: 0.12, mouthUpperUpRight: 0.12, tongueUp: 0.2 },
  и: { jawOpen: 0.22, mouthWide: 0.42, mouthSmileLeft: 0.14, mouthSmileRight: 0.14, cheekSquintLeft: 0.12, cheekSquintRight: 0.12, mouthUpperUpLeft: 0.08, mouthUpperUpRight: 0.08, tongueUp: 0.15 },
  ї: { jawOpen: 0.2, mouthWide: 0.48, mouthSmileLeft: 0.18, mouthSmileRight: 0.18, cheekSquintLeft: 0.15, cheekSquintRight: 0.15, mouthUpperUpLeft: 0.12, mouthUpperUpRight: 0.12, tongueUp: 0.25 },
  е: { jawOpen: 0.3, mouthWide: 0.4, mouthSmileLeft: 0.1, mouthSmileRight: 0.1, cheekSquintLeft: 0.1, cheekSquintRight: 0.1, mouthUpperUpLeft: 0.08, mouthUpperUpRight: 0.08, browInnerUp: 0.06, tongueUp: 0.12 },
  e: { jawOpen: 0.3, mouthWide: 0.4, mouthSmileLeft: 0.1, mouthSmileRight: 0.1, cheekSquintLeft: 0.1, cheekSquintRight: 0.1, mouthUpperUpLeft: 0.08, mouthUpperUpRight: 0.08, browInnerUp: 0.06, tongueUp: 0.12 },
  є: { jawOpen: 0.28, mouthWide: 0.42, mouthSmileLeft: 0.14, mouthSmileRight: 0.14, cheekSquintLeft: 0.12, cheekSquintRight: 0.12, mouthUpperUpLeft: 0.1, mouthUpperUpRight: 0.1, browInnerUp: 0.06, tongueUp: 0.18 },
  м: { mouthClose: 0.95, mouthShrugLower: 0.2, jawOpen: 0, tongueUp: 0 },
  m: { mouthClose: 0.95, mouthShrugLower: 0.2, jawOpen: 0, tongueUp: 0 },
  б: { mouthClose: 0.9, mouthShrugLower: 0.22, jawOpen: 0, tongueUp: 0 },
  b: { mouthClose: 0.9, mouthShrugLower: 0.22, jawOpen: 0, tongueUp: 0 },
  п: { mouthClose: 0.94, mouthShrugLower: 0.2, jawOpen: 0, tongueUp: 0 },
  p: { mouthClose: 0.94, mouthShrugLower: 0.2, jawOpen: 0, tongueUp: 0 },
  ф: { jawOpen: 0.1, mouthWide: 0.22, mouthClose: 0.3, mouthShrugLower: 0.42, mouthUpperUpLeft: 0.12, mouthUpperUpRight: 0.12, tongueUp: 0 },
  f: { jawOpen: 0.1, mouthWide: 0.22, mouthClose: 0.3, mouthShrugLower: 0.42, mouthUpperUpLeft: 0.12, mouthUpperUpRight: 0.12, tongueUp: 0 },
  в: { jawOpen: 0.1, mouthWide: 0.22, mouthClose: 0.28, mouthShrugLower: 0.38, mouthUpperUpLeft: 0.12, mouthUpperUpRight: 0.12, tongueUp: 0 },
  v: { jawOpen: 0.1, mouthWide: 0.22, mouthClose: 0.28, mouthShrugLower: 0.38, mouthUpperUpLeft: 0.12, mouthUpperUpRight: 0.12, tongueUp: 0 },
  л: { jawOpen: 0.2, mouthWide: 0.24, mouthUpperUpLeft: 0.1, mouthUpperUpRight: 0.1, tongueUp: 0.88 },
  l: { jawOpen: 0.2, mouthWide: 0.24, mouthUpperUpLeft: 0.1, mouthUpperUpRight: 0.1, tongueUp: 0.88 },
  н: { jawOpen: 0.18, mouthWide: 0.26, mouthUpperUpLeft: 0.08, mouthUpperUpRight: 0.08, tongueUp: 0.82 },
  n: { jawOpen: 0.18, mouthWide: 0.26, mouthUpperUpLeft: 0.08, mouthUpperUpRight: 0.08, tongueUp: 0.82 },
  д: { jawOpen: 0.18, mouthWide: 0.24, mouthUpperUpLeft: 0.1, mouthUpperUpRight: 0.1, tongueUp: 0.85 },
  d: { jawOpen: 0.18, mouthWide: 0.24, mouthUpperUpLeft: 0.1, mouthUpperUpRight: 0.1, tongueUp: 0.85 },
  т: { jawOpen: 0.18, mouthWide: 0.24, mouthUpperUpLeft: 0.1, mouthUpperUpRight: 0.1, tongueUp: 0.85 },
  t: { jawOpen: 0.18, mouthWide: 0.24, mouthUpperUpLeft: 0.1, mouthUpperUpRight: 0.1, tongueUp: 0.85 },
  с: { jawOpen: 0.12, mouthWide: 0.36, mouthUpperUpLeft: 0.14, mouthUpperUpRight: 0.14, tongueUp: 0.28 },
  s: { jawOpen: 0.12, mouthWide: 0.36, mouthUpperUpLeft: 0.14, mouthUpperUpRight: 0.14, tongueUp: 0.28 },
  з: { jawOpen: 0.14, mouthWide: 0.34, mouthUpperUpLeft: 0.12, mouthUpperUpRight: 0.12, tongueUp: 0.25 },
  z: { jawOpen: 0.14, mouthWide: 0.34, mouthUpperUpLeft: 0.12, mouthUpperUpRight: 0.12, tongueUp: 0.25 },
  ц: { jawOpen: 0.14, mouthWide: 0.36, mouthUpperUpLeft: 0.14, mouthUpperUpRight: 0.14, tongueUp: 0.35 },
  c: { jawOpen: 0.14, mouthWide: 0.36, mouthUpperUpLeft: 0.14, mouthUpperUpRight: 0.14, tongueUp: 0.35 },
  ш: { jawOpen: 0.22, mouthFunnel: 0.36, mouthPucker: 0.26, mouthUpperUpLeft: 0.08, mouthUpperUpRight: 0.08, tongueUp: 0.2 },
  ж: { jawOpen: 0.22, mouthFunnel: 0.36, mouthPucker: 0.24, mouthUpperUpLeft: 0.08, mouthUpperUpRight: 0.08, tongueUp: 0.2 },
  ч: { jawOpen: 0.24, mouthFunnel: 0.38, mouthPucker: 0.24, mouthUpperUpLeft: 0.1, mouthUpperUpRight: 0.1, tongueUp: 0.4 },
  щ: { jawOpen: 0.24, mouthFunnel: 0.36, mouthPucker: 0.26, mouthUpperUpLeft: 0.1, mouthUpperUpRight: 0.1, tongueUp: 0.3 },
  р: { jawOpen: 0.22, mouthWide: 0.22, mouthUpperUpLeft: 0.1, mouthUpperUpRight: 0.1, tongueUp: 0.4 },
  r: { jawOpen: 0.22, mouthWide: 0.22, mouthUpperUpLeft: 0.1, mouthUpperUpRight: 0.1, tongueUp: 0.4 },
  к: { jawOpen: 0.26, mouthWide: 0.2, mouthUpperUpLeft: 0.06, mouthUpperUpRight: 0.06, tongueUp: 0 },
  k: { jawOpen: 0.26, mouthWide: 0.2, mouthUpperUpLeft: 0.06, mouthUpperUpRight: 0.06, tongueUp: 0 },
  г: { jawOpen: 0.26, mouthWide: 0.18, mouthUpperUpLeft: 0.06, mouthUpperUpRight: 0.06, tongueUp: 0 },
  g: { jawOpen: 0.26, mouthWide: 0.18, mouthUpperUpLeft: 0.06, mouthUpperUpRight: 0.06, tongueUp: 0 },
  ґ: { jawOpen: 0.26, mouthWide: 0.18, mouthUpperUpLeft: 0.06, mouthUpperUpRight: 0.06, tongueUp: 0 },
  х: { jawOpen: 0.28, mouthWide: 0.18, tongueUp: 0 },
  h: { jawOpen: 0.28, mouthWide: 0.18, tongueUp: 0 },
  ' ': { jawOpen: 0.02, mouthClose: 0, mouthWide: 0, mouthPucker: 0, mouthFunnel: 0, mouthShrugLower: 0, tongueUp: 0, browInnerUp: 0 },
};

const SHAPES_LIST = [
  'jawOpen', 'mouthWide', 'mouthSmileLeft', 'mouthSmileRight', 'mouthPucker',
  'mouthFunnel', 'mouthClose', 'eyeBlinkLeft', 'eyeBlinkRight', 'browInnerUp',
  'browDown', 'browOuterUp', 'eyeLookUp', 'eyeLookSide', 'tongueUp',
  'browDownLeft', 'browDownRight', 'browOuterUpLeft', 'browOuterUpRight',
  'browInnerUpLeft', 'browInnerUpRight', 'mouthFrownLeft', 'mouthFrownRight',
  'cheekSquintLeft', 'cheekSquintRight', 'eyeWideLeft', 'eyeWideRight',
  'mouthUpperUpLeft', 'mouthUpperUpRight', 'mouthShrugLower',
];

function parseTextToTimeline(text: string, speed = 0.95) {
  const speedScale = 1.0 / Math.max(0.3, speed);
  const timeline: Array<{
    time: number;
    viseme: Record<string, number>;
    wordIdx: number;
    char?: string;
    isPause?: boolean;
    isCue: boolean;
    isWordStart?: boolean;
  }> = [];
  const charTimeline: Array<{
    char: string;
    time: number;
    isPrintable: boolean;
  }> = [];

  let curTime = 0.05 * speedScale;
  const tokenRegex = new RegExp('[\\p{L}\\p{N}]+|[^\\s\\p{L}\\p{N}]+|\\s+', 'gu');
  const tokens = text.match(tokenRegex) || [text];
  let wordIndex = 0;
  const VOWELS = 'аоуіиеєяюaouiey';

  tokens.forEach((tok) => {
    if (/^\s+$/.test(tok)) {
      curTime += 0.08 * speedScale;
      timeline.push({ time: curTime, viseme: VISEME_MAP[' '], wordIdx: wordIndex, isPause: true, isCue: false });
      charTimeline.push({ char: ' ', time: curTime, isPrintable: false });
    } else if (/^[.,!?;:—-]+$/.test(tok)) {
      const punctDur = (0.16 + tok.length * 0.06) * speedScale;
      for (let i = 0; i < tok.length; i++) {
        curTime += punctDur / tok.length;
        charTimeline.push({ char: tok[i], time: curTime, isPrintable: true });
      }
      timeline.push({ time: curTime, viseme: VISEME_MAP[' '], wordIdx: wordIndex, isPause: true, isCue: false });
    } else {
      const lower = tok.toLowerCase();
      let hasVowelInWord = false;
      for (let i = 0; i < lower.length; i++) {
        const char = lower[i];
        const rawChar = tok[i];
        let dur = 0.075;
        const isVowel = VOWELS.includes(char);
        if (isVowel) {
          dur = 0.12;
          hasVowelInWord = true;
        } else if ('мбпmbp'.includes(char)) {
          dur = 0.085;
        } else if ('лндтlndth'.includes(char)) {
          dur = 0.09;
        }
        dur *= speedScale;
        curTime += dur;
        const v = VISEME_MAP[char] || VISEME_MAP[' '];
        const isCue = isVowel || (i === 0 && !hasVowelInWord);

        timeline.push({
          time: curTime - dur * 0.45,
          viseme: v,
          wordIdx: wordIndex,
          char,
          isCue,
          isWordStart: i === 0,
        });
        charTimeline.push({
          char: rawChar,
          time: curTime - dur * 0.45,
          isPrintable: true,
        });
      }
      wordIndex++;
      curTime += 0.04 * speedScale;
    }
  });

  curTime += 0.18 * speedScale;
  timeline.push({ time: curTime, viseme: VISEME_MAP[' '], wordIdx: -1, isPause: true, isCue: false });
  return { timeline, charTimeline, totalDuration: curTime };
}

const MASCOT_EMOTIONS = [
  { id: 'Idle', uk: 'Нейтральна (Idle)', en: 'Neutral (Idle)' },
  { id: 'Smile', uk: 'Посмішка (Smile)', en: 'Smile' },
  { id: 'Thinking', uk: 'Роздуми (Thinking)', en: 'Thinking' },
  { id: 'Surprise', uk: 'Здивування (Surprise)', en: 'Surprise' },
  { id: 'Skeptical', uk: 'Скептична (Skeptical)', en: 'Skeptical' },
  { id: 'Shy', uk: 'Соромʼязлива (Shy)', en: 'Shy' },
  { id: 'Smirk', uk: 'Усмішка (Smirk)', en: 'Smirk' },
  { id: 'Laugh', uk: 'Сміх (Laugh)', en: 'Laugh' },
  { id: 'Wink', uk: 'Підморгування (Wink)', en: 'Wink' },
  { id: 'Sad', uk: 'Сумна (Sad)', en: 'Sad' },
  { id: 'Anger', uk: 'Сердита (Anger)', en: 'Anger' },
  { id: 'Disgust', uk: 'Огида (Disgust)', en: 'Disgust' },
  { id: 'Fear', uk: 'Переляк (Fear)', en: 'Fear' },
];

export const ChatRoom: React.FC<ChatRoomProps> = ({
  interviewId,
  initialDirection = 'Frontend',
  initialLevel = 'Junior',
  initialLanguage,
}) => {
  const router = useRouter();
  const { t, lang } = useTranslation();
  const { interviewLayout, setInterviewLayout } = useAuthStore();
  const sessionLanguage = initialLanguage || lang;

  const [messages, setMessages] = useState<InterviewMessage[]>([]);
  const [input, setInput] = useState('');
  const [loading, setLoading] = useState(false);
  const [completing, setCompleting] = useState(false);
  const [error] = useState<string | null>(null);
  const scrollRef = useRef<HTMLDivElement>(null);

  // Minimalist Mascot, Emotion & Sound state
  const [modelReady, setModelReady] = useState(false);
  const [isSpeaking, setIsSpeaking] = useState(false);
  const [typedSubtitle, setTypedSubtitle] = useState('');
  const [activeEmotion, setActiveEmotion] = useState('Idle');
  const activeEmotionRef = useRef('Idle');
  const [activeSound, setActiveSound] = useState('text_print_soft.wav');
  const [soundVolume, setSoundVolume] = useState(0.7);
  const [isMuted, setIsMuted] = useState(false);
  const prevVolumeRef = useRef(0.7);
  const [showDebugMenu, setShowDebugMenu] = useState(false);

  // Voice, Tempo, Rhythm & Animation Controls
  const [speechSpeed, setSpeechSpeed] = useState(1.0);
  const [soundCadence, setSoundCadence] = useState(0.18);
  const [pitchVariation, setPitchVariation] = useState(0.06);
  const [mouthAmplitude, setMouthAmplitude] = useState(1.0);
  const [headMotionIntensity, setHeadMotionIntensity] = useState(1.0);
  const [activeTexturePreset, setActiveTexturePreset] = useState<string>('realistic');
  const activeTexturePresetRef = useRef<string>('realistic');

  const videoTileRef = useRef<HTMLDivElement>(null);
  const canvasMountRef = useRef<HTMLDivElement>(null);

  const engineRef = useRef<{
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    model?: any;
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    mixer?: any;
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    actions: Record<string, any>;
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    currentAction: any;
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    materialsByName: Record<string, any>;
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    THREE: any;
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    texLoader: any;
    // eslint-disable-next-line @typescript-eslint/no-explicit-any
    textureCache: Map<string, any>;
    speechActive: boolean;
    speechStartTime: number;
    speechDuration: number;
    speechTone: 'statement' | 'question' | 'exclamation';
    speechTimeline: ReturnType<typeof parseTextToTimeline>['timeline'];
    speechCharTimeline: ReturnType<typeof parseTextToTimeline>['charTimeline'];
    currentSpeechVisemes: Record<string, number>;
    speechNodPitch: number;
    speechRoll: number;
    speechYaw: number;
    lastTypingSoundTime: number;
    lastRevealedCharIdx: number;
    activeSound: string;
    soundVolume: number;
    soundCadence: number;
    pitchVariation: number;
    speechSpeed: number;
    mouthAmplitude: number;
    headMotionIntensity: number;
    audioCtx: AudioContext | null;
    audioBuffers: Record<string, AudioBuffer>;
    lastSpokenMsgCount: number;
  }>({
    actions: {},
    currentAction: null,
    materialsByName: {},
    THREE: null,
    texLoader: null,
    textureCache: new Map(),
    speechActive: false,
    speechStartTime: 0,
    speechDuration: 0,
    speechTone: 'statement',
    speechTimeline: [],
    speechCharTimeline: [],
    currentSpeechVisemes: Object.fromEntries(SHAPES_LIST.map((s) => [s, 0])),
    speechNodPitch: 0,
    speechRoll: 0,
    speechYaw: 0,
    lastTypingSoundTime: 0,
    lastRevealedCharIdx: -1,
    activeSound: 'text_print_soft.wav',
    soundVolume: 0.7,
    soundCadence: 0.18,
    pitchVariation: 0.06,
    speechSpeed: 1.0,
    mouthAmplitude: 1.0,
    headMotionIntensity: 1.0,
    audioCtx: null,
    audioBuffers: {},
    lastSpokenMsgCount: 0,
  });

  useEffect(() => {
    engineRef.current.activeSound = isMuted ? 'none' : activeSound;
    engineRef.current.soundVolume = isMuted ? 0 : soundVolume;
    engineRef.current.speechSpeed = speechSpeed;
    engineRef.current.soundCadence = soundCadence;
    engineRef.current.pitchVariation = pitchVariation;
    engineRef.current.mouthAmplitude = mouthAmplitude;
    engineRef.current.headMotionIntensity = headMotionIntensity;
  }, [
    activeSound,
    soundVolume,
    isMuted,
    speechSpeed,
    soundCadence,
    pitchVariation,
    mouthAmplitude,
    headMotionIntensity,
  ]);

  const toggleMute = useCallback(() => {
    setIsMuted((prev) => {
      const next = !prev;
      if (next) {
        if (soundVolume > 0) prevVolumeRef.current = soundVolume;
      } else if (soundVolume === 0) {
        setSoundVolume(prevVolumeRef.current || 0.7);
      }
      return next;
    });
  }, [soundVolume]);

  const getAudioCtx = useCallback(() => {
    const eng = engineRef.current;
    if (!eng.audioCtx && typeof window !== 'undefined') {
      const Ctx =
        window.AudioContext ||
        (window as unknown as { webkitAudioContext: typeof AudioContext }).webkitAudioContext;
      if (Ctx) eng.audioCtx = new Ctx();
    }
    if (eng.audioCtx && eng.audioCtx.state === 'suspended') {
      eng.audioCtx.resume();
    }
    return eng.audioCtx;
  }, []);

  const playSound = useCallback(
    async (filename: string, volumeScale = 1.0, pitchVariation = 0.0) => {
      const eng = engineRef.current;
      if (!filename || filename === 'none' || eng.soundVolume <= 0) return;
      try {
        const ctx = getAudioCtx();
        if (!ctx) return;
        if (!eng.audioBuffers[filename]) {
          const resp = await fetch(SOUNDS_BASE + filename);
          if (!resp.ok) return;
          const arr = await resp.arrayBuffer();
          eng.audioBuffers[filename] = await ctx.decodeAudioData(arr);
        }
        const buffer = eng.audioBuffers[filename];
        if (!buffer) return;

        const source = ctx.createBufferSource();
        source.buffer = buffer;
        if (pitchVariation > 0) {
          source.playbackRate.value = 1.0 + (Math.random() * 2 - 1) * pitchVariation;
        }
        const gain = ctx.createGain();
        gain.gain.value = Math.max(0, Math.min(1, eng.soundVolume * volumeScale));
        source.connect(gain);
        gain.connect(ctx.destination);
        source.start(0);
      } catch {
        // Ignore audio decode/autoplay errors
      }
    },
    [getAudioCtx]
  );

  const playClip = useCallback((name: string) => {
    const eng = engineRef.current;
    if (!eng.mixer || !eng.actions[name]) return;
    const next = eng.actions[name];
    if (eng.currentAction && eng.currentAction !== next) {
      eng.currentAction.fadeOut(0.24);
    }
    next.reset().fadeIn(0.24).play();
    eng.currentAction = next;
  }, []);

  const handleEmotionChange = useCallback(
    (emotion: string) => {
      setActiveEmotion(emotion);
      activeEmotionRef.current = emotion;
      playClip(emotion);
    },
    [playClip]
  );

  const applyTexturePreset = useCallback((presetId: string) => {
    const eng = engineRef.current;
    if (!eng.THREE || !eng.materialsByName || !eng.texLoader) return;
    const preset = TEXTURE_PRESETS.find((p) => p.id === presetId);
    if (!preset) return;
    applyPresetToMaterials(eng.THREE, eng.materialsByName, preset, eng.textureCache, eng.texLoader);
  }, []);

  const handleTextureChange = useCallback(
    (presetId: string) => {
      setActiveTexturePreset(presetId);
      activeTexturePresetRef.current = presetId;
      applyTexturePreset(presetId);
    },
    [applyTexturePreset]
  );

  const speakTextWithMascot = useCallback(
    (text: string) => {
      const eng = engineRef.current;
      getAudioCtx();

      if (/\?\s*$/.test(text)) eng.speechTone = 'question';
      else if (/!\s*$/.test(text)) eng.speechTone = 'exclamation';
      else eng.speechTone = 'statement';

      const currentSpeed = eng.speechSpeed || 1.0;
      const { timeline, charTimeline, totalDuration } = parseTextToTimeline(text, currentSpeed);
      eng.speechTimeline = timeline;
      eng.speechCharTimeline = charTimeline;
      eng.speechDuration = totalDuration;
      eng.lastTypingSoundTime = 0;
      eng.lastRevealedCharIdx = -1;

      const baseEmotion = activeEmotionRef.current || 'Idle';
      if (eng.actions[baseEmotion]) playClip(baseEmotion);
      else if (eng.actions['Idle']) playClip('Idle');

      setTypedSubtitle('');
      setIsSpeaking(true);

      eng.speechStartTime = performance.now() / 1000.0;
      eng.speechActive = true;
    },
    [getAudioCtx, playClip]
  );

  // Initialize Three.js Scene & Load v12 Bust_animated.glb
  useEffect(() => {
    let disposed = false;
    let animFrameId = 0;
    let resizeObserver: ResizeObserver | null = null;

    async function initThree() {
      const mount = canvasMountRef.current;
      const tile = videoTileRef.current;
      if (!mount || !tile) return;

      const dynamicImport = new Function('u', 'return import(u)');
      const [THREE, { GLTFLoader }, { OrbitControls }] = await Promise.all([
        dynamicImport('https://esm.sh/three@0.160.0'),
        dynamicImport('https://esm.sh/three@0.160.0/examples/jsm/loaders/GLTFLoader.js'),
        dynamicImport('https://esm.sh/three@0.160.0/examples/jsm/controls/OrbitControls.js'),
      ]);
      if (disposed) return;

      const eng = engineRef.current;

      const scene = new THREE.Scene();
      scene.background = new THREE.Color(0x0b0b0f);

      const rect = mount.getBoundingClientRect();
      const initW = Math.max(Math.floor(rect.width), 360);
      const initH = Math.max(Math.floor(rect.height), 320);

      // Framed close enough so the bottom of the bust is always off-screen and never floating in mid-air
      const BASE_VFOV = 38;
      const computeAdaptiveFov = (aspect: number) => {
        if (aspect < 0.75) {
          const rad = (BASE_VFOV * Math.PI) / 180;
          const adapted = 2 * Math.atan(Math.tan(rad / 2) * (0.75 / Math.max(0.42, aspect)));
          return Math.min(44, (adapted * 180) / Math.PI);
        }
        return BASE_VFOV;
      };
      // Distance clamped so the bottom cut of the bust never lifts into view
      const computeCameraZ = (aspect: number) => (aspect < 0.75 ? 4.65 : 4.45);

      const camera = new THREE.PerspectiveCamera(
        computeAdaptiveFov(initW / initH),
        initW / initH,
        0.1,
        50
      );
      // Camera y is 1.16, Target y is 1.18: positions the mascot higher in frame to eliminate empty top void while keeping bust bottom submerged
      camera.position.set(0, 1.16, computeCameraZ(initW / initH));

      const renderer = new THREE.WebGLRenderer({ antialias: true });
      renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
      renderer.setSize(initW, initH, true);
      renderer.shadowMap.enabled = true;
      renderer.shadowMap.type = THREE.PCFSoftShadowMap;
      renderer.toneMapping = THREE.ACESFilmicToneMapping;
      renderer.toneMappingExposure = 1.06;
      mount.innerHTML = '';
      mount.appendChild(renderer.domElement);

      // Orbit controls: allows smooth rotation without mouse-wheel zoom detachment
      const controls = new OrbitControls(camera, renderer.domElement);
      controls.enableDamping = true;
      controls.dampingFactor = 0.08;
      controls.enableZoom = false;
      controls.minDistance = 4.2;
      controls.maxDistance = 4.7;
      controls.enablePan = false;
      controls.enableRotate = true;
      controls.rotateSpeed = 0.85;
      controls.minPolarAngle = Math.PI * 0.44;
      controls.maxPolarAngle = Math.PI * 0.50; // prevents pitching camera so low that the underside shows
      controls.minAzimuthAngle = -0.45;
      controls.maxAzimuthAngle = 0.45;
      // Target positioned at y=1.18, elevating the mascot head to reduce top gap
      controls.target.set(0, 1.18, 0.45);
      controls.update();

      eng.THREE = THREE;
      eng.texLoader = new THREE.TextureLoader();
      eng.textureCache = new Map();

      // eslint-disable-next-line @typescript-eslint/no-explicit-any
      (window as any).__mascotDebug = { camera, controls, scene, renderer, playClip, applyTexturePreset };

      let lastW = initW;
      let lastH = initH;
      const syncSize = () => {
        if (!mount) return;
        const w = mount.clientWidth;
        const h = mount.clientHeight;
        if (w > 10 && h > 10 && (w !== lastW || h !== lastH)) {
          lastW = w;
          lastH = h;
          const aspect = w / h;
          camera.aspect = aspect;
          camera.fov = computeAdaptiveFov(aspect);
          const targetZ = computeCameraZ(aspect);
          if (Math.abs(camera.position.z - targetZ) > 0.05) {
            camera.position.z = targetZ;
          }
          camera.updateProjectionMatrix();
          renderer.setSize(w, h, true);
        }
      };
      resizeObserver = new ResizeObserver(syncSize);
      resizeObserver.observe(mount);

      // Studio Three-Point Lighting
      scene.add(new THREE.AmbientLight(0xffffff, 0.28));
      const key = new THREE.DirectionalLight(0xffffff, 1.65);
      key.position.set(3.5, -5, 5);
      key.castShadow = true;
      key.shadow.mapSize.set(2048, 2048);
      key.shadow.camera.left = -4;
      key.shadow.camera.right = 4;
      key.shadow.camera.top = 4;
      key.shadow.camera.bottom = -4;
      key.shadow.camera.near = 0.1;
      key.shadow.camera.far = 20;
      key.shadow.bias = -0.0008;
      scene.add(key);

      const rim = new THREE.DirectionalLight(0x60a5fa, 0.48);
      rim.position.set(-4, 2, 2);
      scene.add(rim);

      const gltfLoader = new GLTFLoader();
      gltfLoader.load(
        MASCOT_BASE + 'Bust_animated.glb',
        // eslint-disable-next-line @typescript-eslint/no-explicit-any
        (gltf: any) => {
          if (disposed) return;
          const model = gltf.scene;
          scene.add(model);
          eng.model = model;
          eng.mixer = new THREE.AnimationMixer(model);
          eng.actions = {};
          // eslint-disable-next-line @typescript-eslint/no-explicit-any
          gltf.animations.forEach((clip: any) => {
            eng.actions[clip.name] = eng.mixer.clipAction(clip);
          });

          eng.materialsByName = {};
          // eslint-disable-next-line @typescript-eslint/no-explicit-any
          model.traverse((child: any) => {
            if (child.isMesh) {
              child.castShadow = true;
              child.receiveShadow = true;
              if (Array.isArray(child.material)) {
                // eslint-disable-next-line @typescript-eslint/no-explicit-any
                child.material.forEach((mat: any) => {
                  eng.materialsByName[mat.name] = mat;
                });
              } else if (child.material) {
                eng.materialsByName[child.material.name] = child.material;
              }
            }
          });

          // Apply initial texture preset (e.g. realistic)
          const initPreset =
            TEXTURE_PRESETS.find((p) => p.id === activeTexturePresetRef.current) ||
            TEXTURE_PRESETS[0];
          applyPresetToMaterials(
            THREE,
            eng.materialsByName,
            initPreset,
            eng.textureCache,
            eng.texLoader
          );

          playClip(activeEmotionRef.current || 'Idle');
          setModelReady(true);
        }
      );

      ['text_print_soft.wav', 'typewriter_key.wav', 'dialogue_blip_soft.wav', 'dialogue_blip_synth.wav', 'tech_ui_chirp.wav', 'whisper_murmur.wav', 'chime_done.wav'].forEach((s) => {
        fetch(SOUNDS_BASE + s).catch(() => {});
      });

      const clock = new THREE.Clock();
      const animate = () => {
        if (disposed) return;
        animFrameId = requestAnimationFrame(animate);
        syncSize();
        const delta = clock.getDelta();
        if (eng.mixer) eng.mixer.update(delta);

        if (eng.model) {
          const now = performance.now() / 1000.0;
          if (eng.speechActive) {
            const elapsed = now - eng.speechStartTime;
            if (elapsed > eng.speechDuration) {
              eng.speechActive = false;
              setIsSpeaking(false);
              if (eng.speechCharTimeline.length > 0) {
                setTypedSubtitle(eng.speechCharTimeline.map((c) => c.char).join(''));
              }
              playSound('chime_done.wav', 0.5);
              const resumeEmotion = activeEmotionRef.current || 'Idle';
              if (eng.actions[resumeEmotion]) playClip(resumeEmotion);
            } else {
              let revealedCount = 0;
              let newLetters = 0;
              for (let c = eng.lastRevealedCharIdx + 1; c < eng.speechCharTimeline.length; c++) {
                const item = eng.speechCharTimeline[c];
                if (item.time <= elapsed) {
                  eng.lastRevealedCharIdx = c;
                  revealedCount++;
                  if (item.isPrintable && item.char !== ' ') newLetters++;
                } else break;
              }
              if (revealedCount > 0) {
                setTypedSubtitle(
                  eng.speechCharTimeline
                    .slice(0, eng.lastRevealedCharIdx + 1)
                    .map((c) => c.char)
                    .join('')
                );
              }
              if (newLetters > 0 && eng.activeSound !== 'none') {
                if (now - eng.lastTypingSoundTime >= (eng.soundCadence || 0.18) * 0.45) {
                  playSound(eng.activeSound, 0.44, eng.pitchVariation ?? 0.06);
                  eng.lastTypingSoundTime = now;
                }
              }

              // Interpolate current viseme
              let targetViseme: Record<string, number> = { ...VISEME_MAP[' '] };
              if (eng.speechTimeline.length > 0) {
                if (elapsed <= eng.speechTimeline[0].time) {
                  targetViseme = { ...eng.speechTimeline[0].viseme };
                } else if (elapsed < eng.speechTimeline[eng.speechTimeline.length - 1].time) {
                  for (let i = 0; i < eng.speechTimeline.length - 1; i++) {
                    if (elapsed >= eng.speechTimeline[i].time && elapsed <= eng.speechTimeline[i + 1].time) {
                      const segT =
                        (elapsed - eng.speechTimeline[i].time) /
                        Math.max(0.001, eng.speechTimeline[i + 1].time - eng.speechTimeline[i].time);
                      const smoothT = segT * segT * (3.0 - 2.0 * segT);
                      const vA = eng.speechTimeline[i].viseme;
                      const vB = eng.speechTimeline[i + 1].viseme;
                      targetViseme = {};
                      SHAPES_LIST.forEach((s) => {
                        targetViseme[s] = (vA[s] || 0) * (1 - smoothT) + (vB[s] || 0) * smoothT;
                      });
                      break;
                    }
                  }
                }
              }

              // Prosody modulation (? and !)
              const speechProgress = Math.min(1.0, elapsed / Math.max(0.4, eng.speechDuration));
              let qArch = 0.0;
              if (eng.speechTone === 'question' && speechProgress > 0.35) {
                qArch = Math.sin(Math.PI * Math.min(1.0, (speechProgress - 0.35) / 0.65));
                targetViseme['browOuterUpRight'] = (targetViseme['browOuterUpRight'] || 0) + 0.42 * qArch;
                targetViseme['browInnerUpLeft'] = (targetViseme['browInnerUpLeft'] || 0) + 0.24 * qArch;
                targetViseme['mouthSmileRight'] = (targetViseme['mouthSmileRight'] || 0) + 0.12 * qArch;
              } else if (eng.speechTone === 'exclamation') {
                const exPulse = 0.5 * (1.0 + Math.sin(elapsed * 4.0));
                targetViseme['browInnerUp'] = (targetViseme['browInnerUp'] || 0) + 0.28;
                targetViseme['eyeWideLeft'] = 0.22 * exPulse;
                targetViseme['eyeWideRight'] = 0.22 * exPulse;
                targetViseme['jawOpen'] = (targetViseme['jawOpen'] || 0) * 1.15;
              }

              targetViseme['cheekSquintLeft'] = (targetViseme['cheekSquintLeft'] || 0) + 0.08;
              targetViseme['cheekSquintRight'] = (targetViseme['cheekSquintRight'] || 0) + 0.08;
              targetViseme['mouthSmileLeft'] = (targetViseme['mouthSmileLeft'] || 0) + 0.04;
              targetViseme['mouthSmileRight'] = (targetViseme['mouthSmileRight'] || 0) + 0.04;

              const lerpFactor = 1.0 - Math.exp(-26.0 * delta);
              SHAPES_LIST.forEach((s) => {
                eng.currentSpeechVisemes[s] += ((targetViseme[s] || 0) - eng.currentSpeechVisemes[s]) * lerpFactor;
              });

              const isMouthOpen = (eng.currentSpeechVisemes['jawOpen'] || 0) > 0.1;
              const nodBase = 0.015 * Math.sin(elapsed * 4.8) * (isMouthOpen ? 1.0 : 0.45);
              eng.speechNodPitch += (nodBase - eng.speechNodPitch) * lerpFactor;
              const targetRoll = (eng.speechTone === 'question' ? -0.024 * qArch : 0) + 0.01 * Math.sin(elapsed * 1.7);
              const targetYaw = (eng.speechTone === 'question' ? 0.018 * qArch : 0) + 0.008 * Math.cos(elapsed * 1.3);
              eng.speechRoll += (targetRoll - eng.speechRoll) * lerpFactor;
              eng.speechYaw += (targetYaw - eng.speechYaw) * lerpFactor;
            }
          } else {
            const fadeFactor = 1.0 - Math.exp(-14.0 * delta);
            SHAPES_LIST.forEach((s) => {
              eng.currentSpeechVisemes[s] += (0 - eng.currentSpeechVisemes[s]) * fadeFactor;
            });
            eng.speechNodPitch += (0 - eng.speechNodPitch) * fadeFactor;
            eng.speechRoll += (0 - eng.speechRoll) * fadeFactor;
            eng.speechYaw += (0 - eng.speechYaw) * fadeFactor;
          }

          // eslint-disable-next-line @typescript-eslint/no-explicit-any
          eng.model.traverse((child: any) => {
            if (child.isMesh && child.morphTargetDictionary && child.morphTargetInfluences) {
              const closeWeight = eng.currentSpeechVisemes['mouthClose'] || 0;
              SHAPES_LIST.forEach((s) => {
                const idx = child.morphTargetDictionary[s];
                if (idx !== undefined) {
                  let val = eng.currentSpeechVisemes[s];
                  if (s === 'jawOpen') {
                    val = val * (1.0 - 0.85 * closeWeight);
                    if (closeWeight > 0.1) {
                      child.morphTargetInfluences[idx] *= 1.0 - 0.8 * closeWeight;
                    }
                  }
                  if (val > 0.001) {
                    const scaled = Math.min(1.0, val * (eng.mouthAmplitude ?? 1.0));
                    child.morphTargetInfluences[idx] = Math.max(child.morphTargetInfluences[idx], scaled);
                  }
                }
              });
            }
          });

          const motionScale = eng.headMotionIntensity ?? 1.0;
          eng.model.rotation.set(
            eng.speechNodPitch * motionScale,
            eng.speechYaw * motionScale,
            eng.speechRoll * motionScale
          );
        }

        controls.update();
        renderer.render(scene, camera);
      };

      animate();
    }

    initThree();

    return () => {
      disposed = true;
      cancelAnimationFrame(animFrameId);
      if (resizeObserver) resizeObserver.disconnect();
    };
  }, [playClip, playSound]);

  // Trigger mascot speech whenever a new assistant message arrives and model is ready
  useEffect(() => {
    if (!modelReady || messages.length === 0) return;
    const eng = engineRef.current;
    if (messages.length !== eng.lastSpokenMsgCount) {
      eng.lastSpokenMsgCount = messages.length;
      const lastMsg = messages[messages.length - 1];
      if (lastMsg.role === 'assistant') {
        speakTextWithMascot(lastMsg.content);
      }
    }
  }, [messages, modelReady, speakTextWithMascot]);

  // Load existing messages or initialize first message (untouched backend contract)
  useEffect(() => {
    let isMounted = true;

    async function loadData() {
      try {
        const data = await api.interviews.getById(interviewId);
        if (isMounted && data.transcript?.length > 0) {
          setMessages(data.transcript);
          return;
        }
      } catch {
        // Fallback demo initial message if running in offline mode
      }

      if (isMounted) {
        setMessages((prev) => {
          if (prev.length > 0) return prev;
          return [
            {
              role: 'assistant',
              content:
                sessionLanguage === 'uk'
                  ? `Вітаю на технічній співбесіді за напрямком ${initialDirection} (${initialLevel})! Я ваш AI-інтерв'юер. Давайте розпочнемо. Розкажіть коротко про себе та ваш практичний досвід у цій сфері.`
                  : `Welcome to your ${initialDirection} (${initialLevel}) technical interview! I am your AI interviewer. Let's begin. Could you please briefly introduce yourself and share your relevant background?`,
              timestamp: new Date().toISOString(),
            },
          ];
        });
      }
    }

    loadData();

    return () => {
      isMounted = false;
    };
  }, [interviewId, initialDirection, initialLevel, sessionLanguage]);

  // Scroll to bottom on new messages
  useEffect(() => {
    if (scrollRef.current) {
      scrollRef.current.scrollTop = scrollRef.current.scrollHeight;
    }
  }, [messages, loading, typedSubtitle]);

  const handleSend = async (e?: React.FormEvent) => {
    if (e) e.preventDefault();
    if (!input.trim() || loading || completing) return;

    getAudioCtx();
    const userText = input.trim();
    setInput('');

    const userMessage: InterviewMessage = {
      role: 'user',
      content: userText,
      timestamp: new Date().toISOString(),
    };

    engineRef.current.speechActive = false;
    setIsSpeaking(false);
    playClip('Thinking');

    setMessages((prev) => [...prev, userMessage]);
    setLoading(true);

    try {
      const response = await api.interviews.sendMessage(interviewId, userText);

      if (response && response.message) {
        setMessages((prev) => [
          ...prev,
          {
            role: 'assistant',
            content: response.message,
            timestamp: new Date().toISOString(),
          },
        ]);
      }
    } catch {
      // Fallback response for offline prototype demo
      setTimeout(() => {
        const fallbackReplies =
          sessionLanguage === 'uk'
            ? [
                'Дякую за відповідь! Скажіть, будь ласка, як саме ви оптимізуєте продуктивність та уникаєте зайвих ререндерів у ваших компонентах?',
                'Чудово. А які патерни обробки помилок та асинхронних запитів ви зазвичай використовуєте на практиці?',
                'Цікавий підхід. Чи доводилося вам налаштовувати CI/CD або працювати з Docker контейнерами для розгортання?',
                'Дякую за ґрунтовні відповіді! Усі питання цієї сесії вичерпано. Тепер ви можете натиснути кнопку завершення, щоб отримати детальний скоркарт.',
              ]
            : [
                'Thank you for your answer! Could you elaborate on how you optimize performance and prevent unnecessary re-renders in your code?',
                'Great point. What patterns do you typically apply for handling async states and API error boundaries in production?',
                'Interesting approach. Have you had hands-on experience configuring Docker or CI/CD pipelines?',
                'Thank you for articulating your reasoning. All questions for this session are complete! You can now finish the interview to view your full scorecard.',
              ];

        const userCount = messages.filter((m) => m.role === 'user').length;
        const nextReply = fallbackReplies[Math.min(userCount, fallbackReplies.length - 1)];

        setMessages((prev) => [
          ...prev,
          {
            role: 'assistant',
            content: nextReply,
            timestamp: new Date().toISOString(),
          },
        ]);
        setLoading(false);
      }, 900);
      return;
    } finally {
      setLoading(false);
    }
  };

  const handleKeyDown = (e: React.KeyboardEvent<HTMLTextAreaElement>) => {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault();
      handleSend();
    }
  };

  const handleComplete = async () => {
    setCompleting(true);

    try {
      await api.interviews.complete(interviewId);
      router.push(`/interview/${interviewId}/report`);
    } catch {
      router.push(`/interview/${interviewId}/report`);
    } finally {
      setCompleting(false);
    }
  };

  const handleTestSpeech = useCallback(() => {
    const phrasesUk = [
      'Вітаю! Я уважно слухаю вашу відповідь на технічній співбесіді.',
      'Чудово! Як саме ви оптимізуєте роботу з компонентами?',
      'Дякую за ґрунтовну відповідь, переходимо до наступного запитання!',
    ];
    const phrasesEn = [
      'Welcome! I am attentively listening to your interview answer.',
      'Great! How do you optimize component rendering in production?',
      'Thank you for the detailed reasoning, let us proceed to the next question!',
    ];
    const pool = sessionLanguage === 'uk' ? phrasesUk : phrasesEn;
    const sample = pool[Math.floor(Math.random() * pool.length)];
    speakTextWithMascot(sample);
  }, [sessionLanguage, speakTextWithMascot]);

  const handleResetSettings = useCallback(() => {
    setSpeechSpeed(1.0);
    setSoundCadence(0.18);
    setPitchVariation(0.06);
    setMouthAmplitude(1.0);
    setHeadMotionIntensity(1.0);
    setSoundVolume(0.7);
    setActiveEmotion('Idle');
    handleEmotionChange('Idle');
    handleTextureChange('realistic');
  }, [handleEmotionChange, handleTextureChange]);

  const userAnswersCount = messages.filter((m) => m.role === 'user').length;
  const lastAssistantIdx = messages.map((m) => m.role).lastIndexOf('assistant');

  return (
    <div className="flex flex-col h-[calc(100vh-4rem)] max-h-[calc(100vh-4rem)] w-full max-w-[1700px] mx-auto px-3 sm:px-6 py-2.5 font-sans overflow-hidden">
      {/* Top Session Minimalist Bar */}
      <div className="flex items-center justify-between gap-3 border-b border-white/[0.08] pb-2.5 mb-2.5 shrink-0 flex-wrap">
        <div className="flex items-center gap-3">
          <div className="flex h-9 w-9 items-center justify-center rounded-xl bg-gradient-to-tr from-rose-500/20 via-indigo-500/10 to-blue-500/20 text-rose-300 border border-rose-500/30 shadow-sm">
            <Bot className="h-4.5 w-4.5" />
          </div>
          <div>
            <div className="flex items-center gap-2">
              <h2 className="text-sm sm:text-base font-semibold text-zinc-100">{t.chat.title}</h2>
              <span className="rounded-md bg-rose-500/15 px-2 py-0.5 text-[11px] font-mono font-medium text-rose-300 border border-rose-500/30 uppercase">
                {initialDirection}
              </span>
              <span className="rounded-md bg-zinc-800 px-2 py-0.5 text-[11px] font-mono text-zinc-300 border border-white/[0.06]">
                {initialLevel}
              </span>
            </div>
            <div className="flex items-center gap-2 text-xs text-zinc-400 mt-0.5 font-mono">
              <Clock className="h-3.5 w-3.5 text-zinc-500" />
              <span>
                {t.chat.candidateAnswers} {userAnswersCount}
              </span>
            </div>
          </div>
        </div>

        {/* Top Controls: Sound Mute, Layout Switcher, Debug Menu Popover, Finish Button */}
        <div className="flex items-center gap-2 flex-wrap relative">
          {/* Subtle Quick Mute Toggle */}
          <button
            type="button"
            onClick={toggleMute}
            title={isMuted || activeSound === 'none' || soundVolume === 0 ? 'Увімкнути звук' : 'Вимкнути звук (Mute)'}
            aria-label="Toggle mute"
            className="flex items-center justify-center rounded-xl bg-zinc-900/80 border border-white/[0.08] p-2 hover:bg-zinc-800 text-zinc-300 hover:text-white transition-all cursor-pointer shadow-sm"
          >
            {isMuted || activeSound === 'none' || soundVolume === 0 ? (
              <VolumeX className="h-4 w-4 text-zinc-500" />
            ) : (
              <Volume2 className="h-4 w-4 text-rose-400" />
            )}
          </button>

          {/* Clean Layout Switcher Pill */}
          <div className="flex items-center rounded-xl bg-zinc-900/80 border border-white/[0.08] p-0.5 text-xs">
            <button
              type="button"
              onClick={() => setInterviewLayout('center')}
              title={sessionLanguage === 'uk' ? 'Фокус (маскот по центру)' : 'Focus (centered mascot)'}
              className={`flex items-center gap-1.5 px-3 py-1.5 rounded-lg transition-all cursor-pointer ${
                interviewLayout === 'center'
                  ? 'bg-rose-500/20 text-rose-300 font-medium shadow-sm'
                  : 'text-zinc-400 hover:text-zinc-200'
              }`}
            >
              <Maximize2 className="h-3.5 w-3.5" />
              <span className="hidden sm:inline text-xs font-mono">
                {sessionLanguage === 'uk' ? 'Фокус' : 'Focus'}
              </span>
            </button>
            <button
              type="button"
              onClick={() => setInterviewLayout('sections')}
              title={
                sessionLanguage === 'uk'
                  ? 'Секції (маскот вгорі, запитання зліва, відповідь справа)'
                  : 'Sections (mascot top, question left, answer right)'
              }
              className={`flex items-center gap-1.5 px-3 py-1.5 rounded-lg transition-all cursor-pointer ${
                interviewLayout === 'sections'
                  ? 'bg-amber-500/20 text-amber-300 font-medium shadow-sm'
                  : 'text-zinc-400 hover:text-zinc-200'
              }`}
            >
              <LayoutGrid className="h-3.5 w-3.5" />
              <span className="hidden sm:inline text-xs font-mono">
                {sessionLanguage === 'uk' ? 'Секції' : 'Sections'}
              </span>
            </button>
            <button
              type="button"
              onClick={() => setInterviewLayout('split')}
              title={
                sessionLanguage === 'uk'
                  ? 'Чат (маскот зліва, історія справа)'
                  : 'Split Chat (mascot left, transcript right)'
              }
              className={`flex items-center gap-1.5 px-3 py-1.5 rounded-lg transition-all cursor-pointer ${
                interviewLayout === 'split'
                  ? 'bg-blue-500/20 text-blue-300 font-medium shadow-sm'
                  : 'text-zinc-400 hover:text-zinc-200'
              }`}
            >
              <Columns2 className="h-3.5 w-3.5" />
              <span className="hidden sm:inline text-xs font-mono">
                {sessionLanguage === 'uk' ? 'Чат' : 'Split'}
              </span>
            </button>
          </div>

          {/* Minimalist Debug Menu Toggle Button */}
          <button
            type="button"
            onClick={() => setShowDebugMenu((prev) => !prev)}
            title="Параметри тестування (Debug)"
            aria-label="Toggle debug settings"
            className={`flex items-center gap-1.5 rounded-xl border px-2.5 py-1.5 text-xs font-mono transition-all cursor-pointer shadow-sm ${
              showDebugMenu
                ? 'bg-zinc-800 text-rose-300 border-rose-500/40'
                : 'bg-zinc-900/80 text-zinc-400 border-white/[0.08] hover:text-zinc-200 hover:bg-zinc-800'
            }`}
          >
            <SlidersHorizontal className="h-3.5 w-3.5" />
            <span className="hidden md:inline">Debug</span>
          </button>

          {/* Collapsible Debug Popover Panel */}
          {showDebugMenu && (
            <div className="absolute right-0 top-12 z-50 w-80 sm:w-96 max-h-[82vh] overflow-y-auto rounded-2xl bg-zinc-950/95 backdrop-blur-xl border border-white/[0.12] p-4 shadow-2xl space-y-4 animate-in fade-in zoom-in-95 duration-150">
              {/* Header */}
              <div className="flex items-center justify-between pb-2 border-b border-white/[0.08]">
                <div className="flex items-center gap-2 text-xs font-semibold text-zinc-100 font-mono">
                  <SlidersHorizontal className="h-4 w-4 text-rose-400" />
                  <span>Керування мовленням та маскотом</span>
                </div>
                <button
                  type="button"
                  onClick={() => setShowDebugMenu(false)}
                  className="rounded-lg p-1 text-zinc-500 hover:text-zinc-300 hover:bg-zinc-800 transition-colors cursor-pointer"
                >
                  <X className="h-4 w-4" />
                </button>
              </div>

              {/* Action Buttons: Test Voice & Reset Defaults */}
              <div className="flex items-center gap-2">
                <button
                  type="button"
                  onClick={handleTestSpeech}
                  className="flex-1 btn-tactile flex items-center justify-center gap-1.5 rounded-xl bg-gradient-to-r from-rose-600 to-pink-600 px-3 py-2 text-xs font-semibold text-white shadow-md shadow-rose-600/20 hover:from-rose-500 hover:to-pink-500 transition-all cursor-pointer"
                >
                  <Play className="h-3.5 w-3.5 fill-current" />
                  <span>Тест говоріння</span>
                </button>
                <button
                  type="button"
                  onClick={handleResetSettings}
                  title="Скинути до стандартних значень"
                  className="flex items-center justify-center gap-1.5 rounded-xl bg-zinc-900 border border-white/[0.08] px-3 py-2 text-xs font-mono text-zinc-400 hover:text-zinc-200 hover:bg-zinc-800 transition-all cursor-pointer"
                >
                  <RotateCcw className="h-3.5 w-3.5" />
                  <span>Скинути</span>
                </button>
              </div>

              {/* SECTION 1: Темп, ритм та динаміка мовлення */}
              <div className="space-y-3 pt-1">
                <div className="flex items-center gap-1.5 text-[11px] font-semibold text-zinc-300 uppercase tracking-wider font-mono border-b border-white/[0.06] pb-1">
                  <Gauge className="h-3.5 w-3.5 text-blue-400" />
                  <span>Темп і ритм мовлення</span>
                </div>

                {/* Speech Speed / Темп говоріння */}
                <div className="space-y-1">
                  <div className="flex items-center justify-between text-[11px] font-mono text-zinc-400">
                    <span>Темп мовлення (Speed):</span>
                    <span className="text-zinc-100 font-semibold">{speechSpeed.toFixed(2)}x</span>
                  </div>
                  <input
                    type="range"
                    min="0.5"
                    max="2.0"
                    step="0.05"
                    value={speechSpeed}
                    onChange={(e) => setSpeechSpeed(parseFloat(e.target.value))}
                    className="w-full accent-blue-500 cursor-pointer"
                  />
                  <div className="flex justify-between text-[10px] font-mono text-zinc-500">
                    <span>0.5x (Повільно)</span>
                    <span>1.0x (Норма)</span>
                    <span>2.0x (Швидко)</span>
                  </div>
                </div>

                {/* Sound Cadence / Ритм клацань друку */}
                <div className="space-y-1">
                  <div className="flex items-center justify-between text-[11px] font-mono text-zinc-400">
                    <span>Ритм звуку друку (Cadence):</span>
                    <span className="text-zinc-100 font-semibold">{soundCadence.toFixed(2)}s</span>
                  </div>
                  <input
                    type="range"
                    min="0.06"
                    max="0.30"
                    step="0.01"
                    value={soundCadence}
                    onChange={(e) => setSoundCadence(parseFloat(e.target.value))}
                    className="w-full accent-blue-500 cursor-pointer"
                  />
                  <div className="flex justify-between text-[10px] font-mono text-zinc-500">
                    <span>Частий (0.06s)</span>
                    <span>Збалансований</span>
                    <span>Рідкісний (0.30s)</span>
                  </div>
                </div>

                {/* Pitch Jitter / Варіація висоти тону */}
                <div className="space-y-1">
                  <div className="flex items-center justify-between text-[11px] font-mono text-zinc-400">
                    <span>Варіація висоти тону (Pitch Jitter):</span>
                    <span className="text-zinc-100 font-semibold">{Math.round(pitchVariation * 100)}%</span>
                  </div>
                  <input
                    type="range"
                    min="0"
                    max="0.20"
                    step="0.01"
                    value={pitchVariation}
                    onChange={(e) => setPitchVariation(parseFloat(e.target.value))}
                    className="w-full accent-blue-500 cursor-pointer"
                  />
                </div>
              </div>

              {/* SECTION 2: Звукові ефекти друку */}
              <div className="space-y-3 pt-1">
                <div className="flex items-center gap-1.5 text-[11px] font-semibold text-zinc-300 uppercase tracking-wider font-mono border-b border-white/[0.06] pb-1">
                  <Volume2 className="h-3.5 w-3.5 text-rose-400" />
                  <span>Звуковий супровід</span>
                </div>

                {/* Sound Preset Select */}
                <div className="space-y-1">
                  <label className="text-[11px] font-mono text-zinc-400">
                    Звуковий ефект появи тексту:
                  </label>
                  <select
                    value={isMuted ? 'none' : activeSound}
                    onChange={(e) => {
                      const val = e.target.value;
                      if (val === 'none') {
                        setIsMuted(true);
                      } else {
                        setIsMuted(false);
                        setActiveSound(val);
                        if (soundVolume === 0) setSoundVolume(prevVolumeRef.current || 0.7);
                        setTimeout(() => playSound(val, 0.8), 10);
                      }
                    }}
                    className="w-full rounded-xl bg-zinc-900 border border-white/[0.08] px-3 py-1.5 text-xs text-zinc-200 focus:outline-none focus:border-rose-500/50 cursor-pointer"
                  >
                    <option value="text_print_soft.wav">Мʼякий друк</option>
                    <option value="typewriter_key.wav">Клавіатура</option>
                    <option value="dialogue_blip_soft.wav">Мʼякий бліп</option>
                    <option value="dialogue_blip_synth.wav">Синт-бліп</option>
                    <option value="tech_ui_chirp.wav">Чирп</option>
                    <option value="whisper_murmur.wav">Шепіт</option>
                    <option value="none">Без звуку (Mute)</option>
                  </select>
                </div>

                {/* Sound Volume */}
                <div className="space-y-1">
                  <div className="flex items-center justify-between text-[11px] font-mono text-zinc-400">
                    <span>Гучність звуку:</span>
                    <span className="text-zinc-100 font-semibold">{isMuted || activeSound === 'none' ? '0%' : `${Math.round(soundVolume * 100)}%`}</span>
                  </div>
                  <input
                    type="range"
                    min="0"
                    max="1"
                    step="0.05"
                    value={isMuted ? 0 : soundVolume}
                    onChange={(e) => {
                      const v = parseFloat(e.target.value);
                      setSoundVolume(v);
                      if (v > 0) {
                        prevVolumeRef.current = v;
                        setIsMuted(false);
                      } else {
                        setIsMuted(true);
                      }
                    }}
                    className="w-full accent-rose-500 cursor-pointer"
                  />
                </div>
              </div>

              {/* SECTION 3: Міміка та Анімація */}
              <div className="space-y-3 pt-1">
                <div className="flex items-center gap-1.5 text-[11px] font-semibold text-zinc-300 uppercase tracking-wider font-mono border-b border-white/[0.06] pb-1">
                  <Activity className="h-3.5 w-3.5 text-amber-400" />
                  <span>Міміка та рухи маскота</span>
                </div>

                {/* Emotion Selector */}
                <div className="space-y-1">
                  <label className="text-[11px] font-mono text-zinc-400 flex items-center gap-1.5">
                    <Smile className="h-3.5 w-3.5 text-amber-400" />
                    <span>Базова емоція маскота:</span>
                  </label>
                  <select
                    value={activeEmotion}
                    onChange={(e) => handleEmotionChange(e.target.value)}
                    className="w-full rounded-xl bg-zinc-900 border border-white/[0.08] px-3 py-1.5 text-xs text-zinc-200 focus:outline-none focus:border-amber-500/50 cursor-pointer"
                  >
                    {MASCOT_EMOTIONS.map((em) => (
                      <option key={em.id} value={em.id} className="bg-zinc-900">
                        {sessionLanguage === 'uk' ? em.uk : em.en}
                      </option>
                    ))}
                  </select>
                </div>

                {/* Mouth Viseme Amplitude / Амплітуда артикуляції */}
                <div className="space-y-1">
                  <div className="flex items-center justify-between text-[11px] font-mono text-zinc-400">
                    <span>Амплітуда міміки рота (Articul.):</span>
                    <span className="text-zinc-100 font-semibold">{mouthAmplitude.toFixed(2)}x</span>
                  </div>
                  <input
                    type="range"
                    min="0.5"
                    max="1.5"
                    step="0.05"
                    value={mouthAmplitude}
                    onChange={(e) => setMouthAmplitude(parseFloat(e.target.value))}
                    className="w-full accent-amber-500 cursor-pointer"
                  />
                  <div className="flex justify-between text-[10px] font-mono text-zinc-500">
                    <span>0.5x (Стримано)</span>
                    <span>1.0x (Норма)</span>
                    <span>1.5x (Експресивно)</span>
                  </div>
                </div>

                {/* Head Motion Intensity / Динаміка кивків */}
                <div className="space-y-1">
                  <div className="flex items-center justify-between text-[11px] font-mono text-zinc-400">
                    <span>Динаміка рухів голови (Head Motion):</span>
                    <span className="text-zinc-100 font-semibold">{headMotionIntensity.toFixed(1)}x</span>
                  </div>
                  <input
                    type="range"
                    min="0"
                    max="2.0"
                    step="0.1"
                    value={headMotionIntensity}
                    onChange={(e) => setHeadMotionIntensity(parseFloat(e.target.value))}
                    className="w-full accent-amber-500 cursor-pointer"
                  />
                  <div className="flex justify-between text-[10px] font-mono text-zinc-500">
                    <span>0.0x (Нерухомо)</span>
                    <span>1.0x (Природно)</span>
                    <span>2.0x (Активно)</span>
                  </div>
                </div>
              </div>

              {/* SECTION 4: Текстури та стилі маскота (Version 12) */}
              <div className="space-y-2 pt-1">
                <div className="flex items-center gap-1.5 text-[11px] font-semibold text-zinc-300 uppercase tracking-wider font-mono border-b border-white/[0.06] pb-1">
                  <Palette className="h-3.5 w-3.5 text-pink-400" />
                  <span>Текстура маскота (Version 12)</span>
                </div>

                <div className="grid grid-cols-2 gap-1.5">
                  {TEXTURE_PRESETS.map((preset) => {
                    const isActive = activeTexturePreset === preset.id;
                    return (
                      <button
                        key={preset.id}
                        type="button"
                        onClick={() => handleTextureChange(preset.id)}
                        className={`flex items-center gap-2 p-2 rounded-xl text-left border transition-all cursor-pointer ${
                          isActive
                            ? 'bg-rose-500/20 border-rose-500/50 text-zinc-100 shadow-sm'
                            : 'bg-zinc-900/60 border-white/[0.06] text-zinc-400 hover:text-zinc-200 hover:bg-zinc-800'
                        }`}
                      >
                        <span className="text-base shrink-0">{preset.icon}</span>
                        <div className="min-w-0">
                          <div className="text-xs font-medium font-mono truncate">
                            {sessionLanguage === 'uk' ? preset.nameUk : preset.nameEn}
                          </div>
                          <div className="text-[10px] text-zinc-500 truncate">
                            {preset.id}
                          </div>
                        </div>
                      </button>
                    );
                  })}
                </div>

                {/* Selected Texture Description */}
                {(() => {
                  const currentPreset = TEXTURE_PRESETS.find((p) => p.id === activeTexturePreset);
                  if (!currentPreset) return null;
                  return (
                    <div className="p-2.5 rounded-xl bg-zinc-900/80 border border-white/[0.06] text-[11px] text-zinc-300 leading-relaxed font-sans">
                      <span className="font-semibold text-zinc-100">
                        {currentPreset.icon} {sessionLanguage === 'uk' ? currentPreset.nameUk : currentPreset.nameEn}:
                      </span>{' '}
                      {sessionLanguage === 'uk' ? currentPreset.descUk : currentPreset.descEn}
                    </div>
                  );
                })()}
              </div>
            </div>
          )}

          {/* Finish Interview Button */}
          <button
            onClick={handleComplete}
            disabled={completing || userAnswersCount === 0}
            className="btn-tactile flex items-center gap-1.5 sm:gap-2 rounded-xl bg-gradient-to-r from-emerald-600 to-teal-600 px-2.5 sm:px-4 py-1.5 sm:py-2 text-xs font-semibold text-white shadow-md shadow-emerald-600/20 hover:from-emerald-500 hover:to-teal-500 disabled:opacity-40 transition-all cursor-pointer shrink-0"
          >
            {completing ? (
              <span>{t.chat.generatingReport}</span>
            ) : (
              <>
                <CheckCircle className="h-3.5 w-3.5 sm:h-4 sm:w-4 shrink-0" />
                <span className="hidden sm:inline">{t.chat.finishBtn}</span>
                <span className="sm:hidden">{sessionLanguage === 'uk' ? 'Кінець' : 'End'}</span>
              </>
            )}
          </button>
        </div>
      </div>

      {error && (
        <div className="mb-2.5 flex items-center gap-2 rounded-xl bg-red-950/20 border border-red-500/30 p-2.5 text-xs text-red-300 shrink-0">
          <AlertCircle className="h-4 w-4 shrink-0" />
          <span>{error}</span>
        </div>
      )}

      {/* Interview Workspace Container */}
      <div
        className={`flex-1 min-h-0 overflow-hidden ${
          interviewLayout === 'center'
            ? 'flex flex-col max-w-5xl w-full mx-auto gap-3'
            : interviewLayout === 'sections'
              ? 'flex-1 min-h-0 w-full flex flex-col gap-3'
              : 'grid grid-cols-1 lg:grid-cols-12 gap-4'
        }`}
      >
        {/* Minimalistic Mascot Video Window */}
        <div
          className={
            interviewLayout === 'center'
              ? 'flex-1 flex flex-col min-h-[260px] max-h-full overflow-hidden'
              : interviewLayout === 'sections'
                ? 'flex-1 min-h-[260px] w-full flex flex-col overflow-hidden'
                : 'lg:col-span-7 flex flex-col h-full min-h-[280px] max-h-full overflow-hidden'
          }
        >
          <div
            ref={videoTileRef}
            className={`relative flex-1 min-h-0 max-h-full rounded-2xl bg-[#0b0b0f] border overflow-hidden flex flex-col justify-end transition-all ${
              isSpeaking
                ? 'border-rose-500/45 shadow-[0_0_24px_rgba(244,63,94,0.14)]'
                : 'border-white/[0.08]'
            }`}
          >
            {/* 3D WebGL Canvas Mount */}
            <div
              ref={canvasMountRef}
              className="absolute inset-0 w-full h-full overflow-hidden cursor-grab active:cursor-grabbing"
            />

            {/* Subtle soft gradient fade at the very bottom edge so the bust base seamlessly merges into the container */}
            <div className="pointer-events-none absolute inset-x-0 bottom-0 h-16 bg-gradient-to-t from-[#0b0b0f] via-[#0b0b0f]/60 to-transparent" />

            {!modelReady && (
              <div className="relative z-10 m-auto flex flex-col items-center gap-2 text-xs text-zinc-400 pointer-events-none">
                <Sparkles className="h-5 w-5 text-rose-400 animate-spin" />
                <span>Підключення відео...</span>
              </div>
            )}

            {/* Question Bubble inside bottom of Video Window (in 'center' and 'split' layouts) */}
            {interviewLayout !== 'sections' && (
              <div className="relative z-10 p-3.5 pointer-events-none">
                <div className="rounded-xl bg-zinc-950/85 backdrop-blur-md border border-white/[0.08] px-4 py-3 shadow-lg">
                  {loading ? (
                    <div className="flex items-center gap-2 text-xs sm:text-sm text-zinc-300 min-h-[2.25rem]">
                      <Sparkles className="h-4 w-4 text-rose-400 animate-spin shrink-0" />
                      <span>{t.chat.aiThinking}</span>
                    </div>
                  ) : (
                    <p className="text-xs sm:text-sm text-zinc-100 leading-relaxed min-h-[2.25rem]">
                      {typedSubtitle ||
                        (lastAssistantIdx >= 0 ? messages[lastAssistantIdx]?.content : '')}
                      {isSpeaking && (
                        <span className="inline-block w-0.5 h-3.5 bg-rose-400 ml-0.5 align-middle animate-pulse" />
                      )}
                    </p>
                  )}
                </div>
              </div>
            )}
          </div>
        </div>

        {/* --- FORMAT 1: 'center' (Answer Form below) --- */}
        {interviewLayout === 'center' && (
          <form onSubmit={handleSend} className="relative shrink-0">
            <textarea
              rows={2}
              value={input}
              onChange={(e) => setInput(e.target.value)}
              onKeyDown={handleKeyDown}
              placeholder={t.chat.placeholder}
              className="w-full resize-none rounded-2xl border border-white/[0.08] bg-zinc-900/70 p-4 pr-14 text-sm text-zinc-100 placeholder-zinc-500 focus:border-rose-500/50 focus:outline-none focus:ring-1 focus:ring-rose-500/40 transition-colors shadow-sm"
            />
            <button
              type="submit"
              disabled={!input.trim() || loading || completing}
              className="btn-tactile absolute right-3.5 bottom-4 flex h-9 w-9 items-center justify-center rounded-xl bg-rose-500 text-white shadow-md shadow-rose-500/25 hover:bg-rose-400 disabled:opacity-30 transition-all cursor-pointer"
            >
              <Send className="h-4 w-4" />
            </button>
          </form>
        )}

        {/* --- FORMAT 2: 'sections' (Mascot up, Left: Question, Right: Answer) --- */}
        {interviewLayout === 'sections' && (
          <div className="shrink-0 grid grid-cols-1 md:grid-cols-2 gap-3.5 h-[260px] sm:h-[280px] lg:h-[300px] w-full">
            {/* LEFT SECTION: Current Question */}
            <div className="flex flex-col rounded-2xl bg-zinc-950/85 border border-white/[0.08] p-4 shadow-sm overflow-hidden">
              <div className="flex items-center gap-2 mb-2 pb-2 border-b border-white/[0.06] shrink-0">
                <Bot className="h-4 w-4 text-rose-400 shrink-0" />
                <span className="text-xs font-semibold uppercase tracking-wider text-zinc-400 font-mono">
                  {sessionLanguage === 'uk' ? 'Поточне запитання' : 'Current Question'}
                </span>
                {isSpeaking && (
                  <span className="ml-auto inline-flex items-center gap-1.5 text-[11px] text-rose-400 font-mono">
                    <span className="h-1.5 w-1.5 rounded-full bg-rose-400 animate-ping" />
                    {sessionLanguage === 'uk' ? 'Говорить...' : 'Speaking...'}
                  </span>
                )}
              </div>
              <div className="flex-1 overflow-y-auto pr-1">
                {loading ? (
                  <div className="flex items-center gap-2 text-sm text-zinc-400 py-3">
                    <Sparkles className="h-4 w-4 text-rose-400 animate-spin shrink-0" />
                    <span>{t.chat.aiThinking}</span>
                  </div>
                ) : (
                  <p className="text-sm sm:text-base text-zinc-100 leading-relaxed">
                    {typedSubtitle ||
                      (lastAssistantIdx >= 0 ? messages[lastAssistantIdx]?.content : '')}
                    {isSpeaking && (
                      <span className="inline-block w-0.5 h-4 bg-rose-400 ml-1 align-middle animate-pulse" />
                    )}
                  </p>
                )}
              </div>
            </div>

            {/* RIGHT SECTION: Place to put your answer */}
            <div className="flex flex-col rounded-2xl bg-zinc-950/85 border border-white/[0.08] p-4 shadow-sm overflow-hidden">
              <div className="flex items-center justify-between mb-2 pb-2 border-b border-white/[0.06] shrink-0">
                <div className="flex items-center gap-2">
                  <User className="h-4 w-4 text-blue-400 shrink-0" />
                  <span className="text-xs font-semibold uppercase tracking-wider text-zinc-400 font-mono">
                    {sessionLanguage === 'uk' ? 'Ваша відповідь' : 'Your Answer'}
                  </span>
                </div>
                <span className="text-[11px] text-zinc-500 font-mono">
                  Enter ↵ {sessionLanguage === 'uk' ? 'для надсилання' : 'to submit'}
                </span>
              </div>
              <form onSubmit={handleSend} className="flex-1 flex flex-col gap-2 min-h-0">
                <textarea
                  value={input}
                  onChange={(e) => setInput(e.target.value)}
                  onKeyDown={handleKeyDown}
                  placeholder={t.chat.placeholder}
                  className="flex-1 w-full resize-none rounded-xl border border-white/[0.08] bg-zinc-900/70 p-3 text-sm text-zinc-100 placeholder-zinc-500 focus:border-rose-500/50 focus:outline-none focus:ring-1 focus:ring-rose-500/40 transition-colors shadow-inner"
                />
                <div className="flex justify-end shrink-0">
                  <button
                    type="submit"
                    disabled={!input.trim() || loading || completing}
                    className="btn-tactile flex items-center gap-2 rounded-xl bg-rose-500 px-4 py-2 text-xs font-semibold text-white shadow-md shadow-rose-500/25 hover:bg-rose-400 disabled:opacity-30 transition-all cursor-pointer"
                  >
                    <span>{sessionLanguage === 'uk' ? 'Відповісти' : 'Submit'}</span>
                    <Send className="h-3.5 w-3.5" />
                  </button>
                </div>
              </form>
            </div>
          </div>
        )}

        {/* --- FORMAT 3: 'split' (Transcript Chat History on Right) --- */}
        {interviewLayout === 'split' && (
          <div className="lg:col-span-5 flex flex-col h-full min-h-0 max-h-full overflow-hidden">
            <div
              ref={scrollRef}
              className="flex-1 overflow-y-auto space-y-4 pr-2 rounded-2xl bg-zinc-950/70 border border-white/[0.08] p-4 shadow-inner mb-3"
            >
              {messages.map((msg, index) => {
                const isAssistant = msg.role === 'assistant';
                const isLatestAssistant = isAssistant && index === lastAssistantIdx;
                const displayedText =
                  isLatestAssistant && typedSubtitle ? typedSubtitle : msg.content;

                return (
                  <div
                    key={index}
                    className={`flex items-start gap-3 ${isAssistant ? 'justify-start' : 'justify-end'}`}
                  >
                    {isAssistant && (
                      <div className="flex h-8 w-8 shrink-0 items-center justify-center rounded-xl bg-zinc-900 border border-white/[0.08] text-rose-300 shadow-sm">
                        <Bot className="h-4 w-4" />
                      </div>
                    )}

                    <div
                      className={`max-w-[85%] rounded-2xl px-4 py-3 text-sm leading-relaxed ${
                        isAssistant
                          ? 'bg-zinc-900/90 text-zinc-200 border border-white/[0.08] shadow-sm'
                          : 'bg-gradient-to-r from-rose-500 via-pink-500 to-indigo-600 text-white shadow-md shadow-rose-500/15'
                      }`}
                    >
                      <div className="whitespace-pre-wrap">{displayedText}</div>
                      <div
                        className={`mt-1.5 text-[10px] font-mono ${
                          isAssistant ? 'text-zinc-500' : 'text-rose-100/80'
                        }`}
                      >
                        {new Date(msg.timestamp).toLocaleTimeString([], {
                          hour: '2-digit',
                          minute: '2-digit',
                        })}
                      </div>
                    </div>

                    {!isAssistant && (
                      <div className="flex h-8 w-8 shrink-0 items-center justify-center rounded-xl bg-zinc-900 border border-white/[0.08] text-zinc-300 shadow-sm">
                        <User className="h-4 w-4 text-blue-400" />
                      </div>
                    )}
                  </div>
                );
              })}

              {/* Loading / Thinking Indicator */}
              {loading && (
                <div className="flex items-start gap-3">
                  <div className="flex h-8 w-8 shrink-0 items-center justify-center rounded-xl bg-zinc-900 border border-white/[0.08] text-rose-300 animate-pulse">
                    <Bot className="h-4 w-4" />
                  </div>
                  <div className="rounded-2xl bg-zinc-900/90 border border-white/[0.08] px-4 py-3 text-sm text-zinc-300 flex items-center gap-2">
                    <Sparkles className="h-4 w-4 text-rose-400 animate-spin" />
                    <span>{t.chat.aiThinking}</span>
                  </div>
                </div>
              )}
            </div>

            {/* Input Area */}
            <form onSubmit={handleSend} className="relative shrink-0">
              <textarea
                rows={2}
                value={input}
                onChange={(e) => setInput(e.target.value)}
                onKeyDown={handleKeyDown}
                placeholder={t.chat.placeholder}
                className="w-full resize-none rounded-2xl border border-white/[0.08] bg-zinc-900/70 p-4 pr-14 text-sm text-zinc-100 placeholder-zinc-500 focus:border-rose-500/50 focus:outline-none focus:ring-1 focus:ring-rose-500/40 transition-colors shadow-sm"
              />
              <button
                type="submit"
                disabled={!input.trim() || loading || completing}
                className="btn-tactile absolute right-3.5 bottom-4 flex h-9 w-9 items-center justify-center rounded-xl bg-rose-500 text-white shadow-md shadow-rose-500/25 hover:bg-rose-400 disabled:opacity-30 transition-all cursor-pointer"
              >
                <Send className="h-4 w-4" />
              </button>
            </form>
          </div>
        )}
      </div>
    </div>
  );
};
