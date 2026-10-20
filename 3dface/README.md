# 3D Face — low-poly male head assets

Everything in this folder is a glTF 2.0 asset for the Dev Dialogue interview
avatar. The app (Next.js + three.js) loads them with `GLTFLoader`.

## Files

| File | What it is | Tris | Animation clips |
| --- | --- | --- | --- |
| `face.glb` | Procedural low-poly male head (skin, eyes, brows, mouth cavity) | 3 600 | `Idle`, `Nod`, `Turn`, `Talk`, `Blink`, `BrowRaise` |
| `face copy lowpoly.glb` | Scan → 6 k tris + low-poly hair cap + head animation | 8 200 | `Idle`, `Nod`, `Talk`, `Turn` |
| `face.blend`, `face copy lowpoly.blend` | Editable Blender sources | – | – |
| `preview_*.png` | Renders used for review | – | – |

Model conventions: **Y up, +Z is the face direction, origin at the neck base,
head height 0.23 m** (glTF is metres).

## Review page

`index.html` is a zero-build React + three.js page that lists **every model in
this folder** with its size, triangle count and clips, and lets you orbit,
scrub the animation clips, toggle wireframe and drag & drop any other `.glb`.

```bash
node 3dface/scan-models.mjs   # refresh models.json / models.js after adding a model
node 3dface/serve.mjs 5173    # then open http://localhost:5173/
```

`scan-models.mjs` must be re-run whenever a model is added to this folder —
the page reads `models.json` (live) and falls back to the inline `models.js`
when opened straight from disk (`file://`).

## Regenerating the models

Both generators are Blender Python scripts. Run them inside Blender (5.x), e.g.
from the Blender MCP server:

```python
exec(open(r"C:/Github/dev-dialogue/3dface/build_face.py").read(), {"__name__": "__main__"})
exec(open(r"C:/Github/dev-dialogue/3dface/build_facecopy.py").read(), {"__name__": "__main__"})
```

* `build_face.py` — metaball skull → analytic face sculpt (eye sockets, nose,
  brow, lips) → mouth cavity → decimate → armature (`head`, `jaw`) → 6 clips.
* `build_facecopy.py` — imports a scan (`face copy.glb`), normalises
  scale/pivot, decimates, cuts a hair cap from the scalp, adds 4 head-motion
  clips. It is a no-op while the source scan is missing.

Both write `*.glb`, `*.blend` and the preview renders into this folder.

## Using the clips in the app

```js
const gltf = await loader.loadAsync('/3dface/face.glb');
const mixer = new THREE.AnimationMixer(gltf.scene);
const action = mixer.clipAction(gltf.animations.find((c) => c.name === 'Idle'));
action.loop = THREE.LoopRepeat;
action.play();
```

Clip names are exactly the `AnimationClip.name` values listed above.
