// Static design tables (docs/01–03) as data.
#pragma once
#include "Types.h"
#include <cstdlib>
#include <cstdio>

// Fail-fast: every violated invariant aborts with location in every build configuration.
#define GL_CHECK(cond, msg) do { if (!(cond)) { std::fprintf(stderr, "GL_CHECK failed: %s\n  at %s:%d\n  %s\n", #cond, __FILE__, __LINE__, msg); std::fflush(stderr); std::abort(); } } while (0)

namespace gl {

struct SectorDef { const char* name; double cap; double demand; double conduit; int pop; double fw; char glyph; };
struct NodeDef   { const char* name; double raw; double mw; double cost; int build; double upkeep; double fw; };
struct LinkDef   { const char* name; double cap; double cost; double upkeep; bool wireless; bool hidden; bool armored; Tech tech; bool hasTech; Arch arch; bool archOnly; };
struct StructDef { const char* name; double cost; double upkeep; int build; double mw; int range; bool needsNode; bool exchangeOnly; bool industrialOnly; Arch arch; bool archOnly; Tech tech; bool hasTech; double needsP; };
struct OpDef     { const char* name; double bw; double capital; double x; bool sustained; bool own; double needsP; bool backbone; Arch arch; bool archOnly; const char* desc; };
struct TechDef   { Tech id; Branch branch; int tier; double cost; const char* name; const char* desc; };
struct DoctrineDef { Doctrine id; Arch arch; int tier; const char* name; const char* desc; };
struct ArchDef   { const char* name; const char* tag; const char* kpi; };
struct ExecDef   { const char* name; const char* desc; };

const SectorDef& sectorDef(Sector s);
const NodeDef& nodeDef(int tier);           // 1..4
const LinkDef& linkDef(LinkType t);
const StructDef& structDef(StructKind k);
const OpDef& opDef(OpKind k);
const TechDef& techDef(Tech t);
const DoctrineDef& doctrineDef(Doctrine d);
const ArchDef& archDef(Arch a);
const ExecDef& execDef(ExecSpec e);
const char* branchName(Branch b);
const char* pathName(VictoryPath p);
const char* aiName(Arch a, int n);

constexpr int TechCount = (int)Tech::COUNT;
constexpr int DoctrineCount = (int)Doctrine::COUNT;

} // namespace gl
