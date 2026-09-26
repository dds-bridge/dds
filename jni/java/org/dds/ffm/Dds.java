/*
   DDS, a bridge double dummy solver.

   Java Foreign Function & Memory (Project Panama) bindings for the DDS native
   shared library. These bind the pure-C ABI shim (dds_c_*) plus GetDDSInfo from
   the flat C API. Requires JDK 22+ (java.lang.foreign is stable there).

   The bindings are hand-written rather than jextract-generated: jextract ships
   only as non-hermetic early-access binaries, whereas this file needs nothing
   beyond the JDK. It mirrors exactly what jextract would emit — struct
   MemoryLayouts and Linker downcall handles.

   See LICENSE and README.
*/

package org.dds.ffm;

import static java.lang.foreign.ValueLayout.ADDRESS;
import static java.lang.foreign.ValueLayout.JAVA_BYTE;
import static java.lang.foreign.ValueLayout.JAVA_INT;

import java.io.IOException;
import java.io.InputStream;
import java.io.UncheckedIOException;
import java.lang.foreign.Arena;
import java.lang.foreign.FunctionDescriptor;
import java.lang.foreign.Linker;
import java.lang.foreign.MemoryLayout;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.SymbolLookup;
import java.lang.invoke.MethodHandle;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;

/**
 * FFM bindings for the DDS native shared library. Load one instance per library
 * file with {@link #load(Path)}; it owns an {@link Arena} scoping the native
 * library and any segments allocated from {@link #arena()}. Not thread-safe:
 * use one native SolverContext per thread.
 */
public class Dds implements AutoCloseable {

    // ---- Struct memory layouts ----
    //
    // Sizes, offsets and padding match the C structs in library/src/api/dll.h
    // exactly. Field *names* are Java-idiomatic camelCase, so a few differ from
    // the C identifiers when cross-checking against the header:
    //
    //     resTable           <- res_table       (DdTableResults)
    //     parScore           <- par_score       (ParResults)
    //     parContractsString <- par_contracts_string
    //     versionString      <- version_string  (DDSInfo)
    //
    // All other names are identical to the header's.

    /** struct Deal — DDS_HANDS = DDS_SUITS = 4. */
    public static final MemoryLayout DEAL = MemoryLayout.structLayout(
            JAVA_INT.withName("trump"),
            JAVA_INT.withName("first"),
            MemoryLayout.sequenceLayout(3, JAVA_INT).withName("currentTrickSuit"),
            MemoryLayout.sequenceLayout(3, JAVA_INT).withName("currentTrickRank"),
            MemoryLayout.sequenceLayout(16, JAVA_INT).withName("remainCards"))
            .withName("Deal");

    /** struct FutureTricks. */
    public static final MemoryLayout FUTURE_TRICKS = MemoryLayout.structLayout(
            JAVA_INT.withName("nodes"),
            JAVA_INT.withName("cards"),
            MemoryLayout.sequenceLayout(13, JAVA_INT).withName("suit"),
            MemoryLayout.sequenceLayout(13, JAVA_INT).withName("rank"),
            MemoryLayout.sequenceLayout(13, JAVA_INT).withName("equals"),
            MemoryLayout.sequenceLayout(13, JAVA_INT).withName("score"))
            .withName("FutureTricks");

    /** struct DdTableDeal. */
    public static final MemoryLayout DD_TABLE_DEAL = MemoryLayout.structLayout(
            MemoryLayout.sequenceLayout(16, JAVA_INT).withName("cards"))
            .withName("DdTableDeal");

    /** struct DdTableDealPBN — cards is a NUL-terminated PBN deal string. */
    public static final MemoryLayout DD_TABLE_DEAL_PBN = MemoryLayout.structLayout(
            MemoryLayout.sequenceLayout(80, JAVA_BYTE).withName("cards"))
            .withName("DdTableDealPBN");

    /** struct DdTableResults — res_table[DDS_STRAINS][DDS_HANDS] = 5x4. */
    public static final MemoryLayout DD_TABLE_RESULTS = MemoryLayout.structLayout(
            MemoryLayout.sequenceLayout(20, JAVA_INT).withName("resTable"))
            .withName("DdTableResults");

    /** struct ParResults — par_score[2][16], par_contracts_string[2][128]. */
    public static final MemoryLayout PAR_RESULTS = MemoryLayout.structLayout(
            MemoryLayout.sequenceLayout(2 * 16, JAVA_BYTE).withName("parScore"),
            MemoryLayout.sequenceLayout(2 * 128, JAVA_BYTE).withName("parContractsString"))
            .withName("ParResults");

    /** struct DDSInfo. The char[10] version_string forces 2 bytes of padding. */
    public static final MemoryLayout DDS_INFO = MemoryLayout.structLayout(
            JAVA_INT.withName("major"),
            JAVA_INT.withName("minor"),
            JAVA_INT.withName("patch"),
            MemoryLayout.sequenceLayout(10, JAVA_BYTE).withName("versionString"),
            MemoryLayout.paddingLayout(2),
            JAVA_INT.withName("system"),
            JAVA_INT.withName("numBits"),
            JAVA_INT.withName("compiler"),
            JAVA_INT.withName("constructor"),
            JAVA_INT.withName("numCores"),
            JAVA_INT.withName("threading"),
            JAVA_INT.withName("noOfThreads"),
            MemoryLayout.sequenceLayout(128, JAVA_BYTE).withName("threadSizes"),
            MemoryLayout.sequenceLayout(1024, JAVA_BYTE).withName("systemString"))
            .withName("DDSInfo");

    private final Arena arena;
    private final MethodHandle getDdsInfo;
    private final MethodHandle createContext;
    private final MethodHandle destroyContext;
    private final MethodHandle solveBoard;
    private final MethodHandle calcDdTable;
    private final MethodHandle calcPar;
    private final MethodHandle calcDdTablePbn;
    private final MethodHandle convertFromPbn;

    private Dds(Arena arena, SymbolLookup lookup) {
        this.arena = arena;
        Linker linker = Linker.nativeLinker();
        this.getDdsInfo = handle(linker, lookup, "GetDDSInfo",
                FunctionDescriptor.ofVoid(ADDRESS));
        this.createContext = handle(linker, lookup, "dds_c_create_solvercontext_default",
                FunctionDescriptor.of(ADDRESS));
        this.destroyContext = handle(linker, lookup, "dds_c_destroy_solvercontext",
                FunctionDescriptor.ofVoid(ADDRESS));
        this.solveBoard = handle(linker, lookup, "dds_c_solve_board",
                FunctionDescriptor.of(JAVA_INT, ADDRESS, ADDRESS, JAVA_INT, JAVA_INT, JAVA_INT, ADDRESS));
        this.calcDdTable = handle(linker, lookup, "dds_c_calc_dd_table",
                FunctionDescriptor.of(JAVA_INT, ADDRESS, ADDRESS, ADDRESS));
        this.calcPar = handle(linker, lookup, "dds_c_calc_par",
                FunctionDescriptor.of(JAVA_INT, ADDRESS, ADDRESS, JAVA_INT, ADDRESS, ADDRESS));
        this.calcDdTablePbn = handle(linker, lookup, "dds_c_calc_dd_table_pbn",
                FunctionDescriptor.of(JAVA_INT, ADDRESS, ADDRESS, ADDRESS));
        this.convertFromPbn = handle(linker, lookup, "dds_c_convert_from_pbn",
                FunctionDescriptor.of(JAVA_INT, ADDRESS, ADDRESS));
    }

    private static MethodHandle handle(Linker linker, SymbolLookup lookup, String name,
            FunctionDescriptor descriptor) {
        MemorySegment symbol = lookup.find(name)
                .orElseThrow(() -> new IllegalStateException("symbol not found: " + name));
        return linker.downcallHandle(symbol, descriptor);
    }

    /** Load the DDS shared library at {@code libraryFile} and bind its symbols. */
    public static Dds load(Path libraryFile) {
        Arena arena = Arena.ofShared();
        try {
            SymbolLookup lookup = SymbolLookup.libraryLookup(libraryFile, arena);
            return new Dds(arena, lookup);
        } catch (RuntimeException | Error e) {
            // A shared arena is not auto-managed; close it so a failed load (e.g.
            // wrong-arch/missing library, or a missing symbol) does not leak the
            // native memory session for the JVM lifetime.
            arena.close();
            throw e;
        }
    }

    /**
     * Load the DDS library bundled inside this jar for the current OS/arch,
     * extracting it to a temp file first. Requires the native library to be on
     * the classpath at {@code /native/<os>-<arch>/<lib>} (host platform only).
     */
    public static Dds loadEmbedded() {
        String os = osToken();
        String arch = archToken();
        String lib = libFileName(os);
        String resource = "/native/" + os + "-" + arch + "/" + lib;
        try (InputStream in = Dds.class.getResourceAsStream(resource)) {
            if (in == null) {
                throw new IllegalStateException("no embedded DDS library at " + resource);
            }
            String suffix = lib.substring(lib.lastIndexOf('.'));
            String extractDir = System.getProperty("dds.extract.dir");
            Path tmp = (extractDir == null || extractDir.isBlank())
                    ? Files.createTempFile("dds", suffix)
                    : Files.createTempFile(Path.of(extractDir), "dds", suffix);
            Files.copy(in, tmp, StandardCopyOption.REPLACE_EXISTING);
            tmp.toFile().deleteOnExit();
            try {
                return load(tmp);
            } catch (RuntimeException | Error e) {
                try {
                    Files.deleteIfExists(tmp);
                } catch (IOException ignored) {
                    // Best-effort cleanup; deleteOnExit still applies.
                }
                throw e;
            }
        } catch (IOException e) {
            throw new UncheckedIOException(e);
        }
    }

    private static String osToken() {
        String name = System.getProperty("os.name", "").toLowerCase(java.util.Locale.ROOT);
        if (name.contains("mac") || name.contains("darwin")) {
            return "macos";
        }
        if (name.contains("win")) {
            return "windows";
        }
        if (name.contains("linux")) {
            return "linux";
        }
        throw new UnsupportedOperationException(
                "unsupported OS for embedded DDS library: " + System.getProperty("os.name"));
    }

    private static String archToken() {
        String arch = System.getProperty("os.arch", "").toLowerCase(java.util.Locale.ROOT);
        if (arch.equals("aarch64") || arch.equals("arm64")) {
            return "aarch64";
        }
        if (arch.equals("x86_64") || arch.equals("amd64")) {
            return "x86_64";
        }
        throw new UnsupportedOperationException(
                "unsupported architecture for embedded DDS library: " + System.getProperty("os.arch"));
    }

    private static String libFileName(String os) {
        switch (os) {
            case "macos":
                return "libdds.dylib";
            case "windows":
                return "dds.dll";
            default:
                return "libdds.so";
        }
    }

    /** Arena scoping this library; use it to allocate struct segments. */
    public Arena arena() {
        return arena;
    }

    /** Fill {@code info} (a {@link #DDS_INFO} segment) with library metadata. */
    public void getDdsInfo(MemorySegment info) {
        invokeVoid(getDdsInfo, info);
    }

    /** Create a default solver context; returns an opaque native handle. */
    public MemorySegment createSolverContext() {
        try {
            return (MemorySegment) createContext.invoke();
        } catch (Throwable t) {
            throw rethrow(t);
        }
    }

    /** Destroy a solver context returned by {@link #createSolverContext()}. */
    public void destroySolverContext(MemorySegment ctx) {
        invokeVoid(destroyContext, ctx);
    }

    /** Solve a single board. Returns a {@link DdsStatus} {@code RETURN_*} code. */
    public int solveBoard(MemorySegment ctx, MemorySegment deal, int target,
            int solutions, int mode, MemorySegment futureTricks) {
        try {
            return (int) solveBoard.invoke(ctx, deal, target, solutions, mode, futureTricks);
        } catch (Throwable t) {
            throw rethrow(t);
        }
    }

    /** Compute the double dummy table for a deal. Returns a {@link DdsStatus} {@code RETURN_*} code. */
    public int calcDdTable(MemorySegment ctx, MemorySegment deal, MemorySegment results) {
        try {
            return (int) calcDdTable.invoke(ctx, deal, results);
        } catch (Throwable t) {
            throw rethrow(t);
        }
    }

    /** Compute the par result for a deal. Returns a {@link DdsStatus} {@code RETURN_*} code. */
    public int calcPar(MemorySegment ctx, MemorySegment deal, int vulnerable,
            MemorySegment results, MemorySegment par) {
        try {
            return (int) calcPar.invoke(ctx, deal, vulnerable, results, par);
        } catch (Throwable t) {
            throw rethrow(t);
        }
    }

    /** Compute the double dummy table for a PBN-format deal. Returns a {@link DdsStatus} {@code RETURN_*} code. */
    public int calcDdTablePbn(MemorySegment ctx, MemorySegment dealPbn, MemorySegment results) {
        try {
            return (int) calcDdTablePbn.invoke(ctx, dealPbn, results);
        } catch (Throwable t) {
            throw rethrow(t);
        }
    }

    /**
     * Parse a NUL-terminated PBN deal string into a binary holdings block: 16
     * consecutive ints, row-major {@code [hand][suit]}, as laid out by
     * {@link #DEAL}'s {@code remainCards} and {@link #DD_TABLE_DEAL}'s
     * {@code cards}. Point {@code cards} at either field (or a slice of it) and
     * then use the binary entry points; this is the general PBN path, which is
     * why solve and par have no PBN variant here. The string must be shorter
     * than 80 bytes including its terminator. Needs no solver context.
     * Returns a {@link DdsStatus} {@code RETURN_*} code.
     *
     * <p>{@code RETURN_NO_FAULT} means the string parsed, not that it describes
     * a legal deal: an unrecognized rank is skipped rather than refused, so a
     * typo'd rank yields a short hand and still converts — except a compass
     * letter (N/E/S/W, either case) found after the first hand, which IS
     * refused with {@code RETURN_PBN_FAULT} rather than skipped. The solve and
     * table calls validate the deal and return {@code RETURN_CARD_COUNT} or
     * {@code RETURN_DUPLICATE_CARDS}, so check their status too.
     *
     * <p>On failure {@code cards} is not preserved — it is zeroed before parsing
     * begins and may hold a partial parse. Convert into scratch storage if the
     * destination must survive a bad string.
     */
    public int convertFromPbn(MemorySegment pbnDeal, MemorySegment cards) {
        try {
            return (int) convertFromPbn.invoke(pbnDeal, cards);
        } catch (Throwable t) {
            throw rethrow(t);
        }
    }

    @Override
    public void close() {
        arena.close();
    }

    private static void invokeVoid(MethodHandle handle, MemorySegment arg) {
        try {
            handle.invoke(arg);
        } catch (Throwable t) {
            throw rethrow(t);
        }
    }

    private static RuntimeException rethrow(Throwable t) {
        if (t instanceof RuntimeException re) {
            return re;
        }
        if (t instanceof Error err) {
            // Propagate JVM-fatal conditions (OutOfMemoryError, LinkageError,
            // ...) unchanged; wrapping them would hide them behind callers'
            // RuntimeException handling.
            throw err;
        }
        return new RuntimeException(t);
    }
}
