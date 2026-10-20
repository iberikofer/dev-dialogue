/**
 * Scans this folder for models and writes the manifest used by index.html.
 *
 *   node 3dface/scan-models.mjs
 *
 * Outputs:
 *   models.json  — the list, read by index.html at runtime (needs a server)
 *   models.js    — the same list as a global, so index.html also works from
 *                  file:// (drag & drop a .glb to preview anything else)
 */
import { readdirSync, statSync, writeFileSync, readFileSync } from 'node:fs';
import { dirname, join, basename } from 'node:path';
import { fileURLToPath } from 'node:url';

const DIR = dirname(fileURLToPath(import.meta.url));
const MODELS = ['.glb', '.gltf'];

function readGlbMeta(file) {
  const buf = readFileSync(file);
  if (buf.readUInt32LE(0) !== 0x46546c67) return null; // 'glTF'
  const jsonLen = buf.readUInt32LE(12);
  const json = JSON.parse(buf.toString('utf8', 20, 20 + jsonLen));
  const accessors = json.accessors || [];
  const triCount = (json.meshes || []).reduce((sum, mesh) => {
    for (const prim of mesh.primitives || []) {
      const count = prim.indices != null
        ? accessors[prim.indices]?.count
        : accessors[prim.attributes?.POSITION]?.count;
      sum += (count || 0) / 3;
    }
    return sum;
  }, 0);
  return {
    tris: Math.round(triCount),
    animations: (json.animations || []).map((a) => a.name || 'Animation'),
    materials: (json.materials || []).length,
    textured: (json.images || []).length > 0,
    nodes: (json.nodes || []).length,
  };
}

const models = readdirSync(DIR)
  .filter((f) => MODELS.includes(f.slice(f.lastIndexOf('.')).toLowerCase()))
  .filter((f) => !f.startsWith('_'))
  .map((file) => {
    const stat = statSync(join(DIR, file));
    const meta = readGlbMeta(join(DIR, file)) || {};
    return {
      file,
      name: basename(file, file.slice(file.lastIndexOf('.'))),
      size: stat.size,
      sizeLabel: stat.size > 1048576
        ? `${(stat.size / 1048576).toFixed(2)} MB`
        : `${Math.round(stat.size / 1024)} KB`,
      modified: stat.mtime.toISOString().slice(0, 10),
      ...meta,
    };
  })
  .sort((a, b) => a.name.localeCompare(b.name));

writeFileSync(join(DIR, 'models.json'), JSON.stringify(models, null, 2));
writeFileSync(
  join(DIR, 'models.js'),
  `window.__MODELS__ = ${JSON.stringify(models, null, 2)};\n`,
);

console.log(`[3dface] ${models.length} model(s):`);
for (const m of models) {
  console.log(`  - ${m.file}  ${m.sizeLabel}  ${m.tris || '?'} tris  clips: ${(m.animations || []).join(', ') || '-'}`);
}
