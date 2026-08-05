#!/usr/bin/env node
/**
 * System smoke test: load Emscripten MVP module and call dds_mvp_calc_table.
 *
 * Usage: node dds_mvp_wasm_node.mjs EMSCRIPTEN_JS WASM_FILE [PBN]
 */
import fs from "node:fs";
import path from "node:path";
import { createRequire } from "node:module";

const require = createRequire(import.meta.url);

const EXPECTED_DEFAULT = [
  5, 8, 5, 8, 6, 6, 6, 6, 5, 7, 5, 7, 7, 5, 7, 5, 6, 6, 6, 6,
];

const DEFAULT_PBN =
  "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3";

function fail(message) {
  console.error(message);
  process.exit(1);
}

async function main() {
  const [jsPath, wasmPath, pbnArg] = process.argv.slice(2);
  if (!jsPath || !wasmPath) {
    fail("usage: node dds_mvp_wasm_node.mjs EMSCRIPTEN_JS WASM_FILE [PBN]");
  }

  const wasmBinary = fs.readFileSync(wasmPath);
  const createDdsModule = require(path.resolve(jsPath));
  if (typeof createDdsModule !== "function") {
    fail("createDdsModule is not a function");
  }

  const module = await createDdsModule({ wasmBinary });
  const pbn = pbnArg ?? DEFAULT_PBN;
  const outPtr = module._malloc(20 * 4);
  try {
    const rc = module.ccall(
      "dds_mvp_calc_table",
      "number",
      ["string", "number"],
      [pbn, outPtr],
    );
    if (rc !== 1) {
      fail(`dds_mvp_calc_table returned ${rc}`);
    }

    const out = [];
    for (let i = 0; i < 20; i++) {
      out.push(module.getValue(outPtr + i * 4, "i32"));
    }

    for (let i = 0; i < EXPECTED_DEFAULT.length; i++) {
      if (out[i] !== EXPECTED_DEFAULT[i]) {
        fail(`table[${i}] = ${out[i]}, expected ${EXPECTED_DEFAULT[i]}`);
      }
    }
  } finally {
    module._free(outPtr);
  }

  const leadsPtr = module._malloc((1 + 13 * 3) * 4);
  try {
    // North declares NT → East leads.
    const leadRc = module.ccall(
      "dds_mvp_solve_leads",
      "number",
      ["string", "number", "number", "number"],
      [pbn, 4, 1, leadsPtr],
    );
    if (leadRc !== 1) {
      fail(`dds_mvp_solve_leads returned ${leadRc}`);
    }
    const n = module.getValue(leadsPtr, "i32");
    if (n < 1 || n > 13) {
      fail(`dds_mvp_solve_leads card count ${n}`);
    }
    console.log("dds_mvp_wasm_node: OK");
  } finally {
    module._free(leadsPtr);
  }
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
