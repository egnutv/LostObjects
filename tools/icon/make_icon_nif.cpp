// Builds meshes/LostObjects/MenuIcon.nif: a small picture frame for the
// workshop menu icon of Lost Objects. The logo is painted on both faces
// (mirrored UVs on the back, so it reads correctly from both sides); the
// edges use the frame color of the texture.
//
// Build tool only: uses nifly (https://github.com/ousnius/nifly, GPL-3.0),
// which is NOT part of the mod. The generated .nif is our own data.
//
//   make_icon_nif <out.nif>            write the mesh
//   make_icon_nif --check <in.nif>     load a mesh and print what is in it

#include "NifFile.hpp"

#include <cstdio>
#include <string>
#include <vector>

using namespace nifly;

namespace
{
	constexpr float W = 64.0f;  // width (X)
	constexpr float H = 64.0f;  // height (Z, up)
	constexpr float D = 4.0f;   // thickness (Y)
	constexpr float EDGE_U = 0.02f;  // a texel inside the painted frame border
	constexpr float EDGE_V = 0.02f;

	struct Mesh
	{
		std::vector<Vector3> verts;
		std::vector<Vector3> normals;
		std::vector<Vector2> uvs;
		std::vector<Triangle> tris;

		void Quad(const Vector3& a, const Vector3& b, const Vector3& c, const Vector3& d, const Vector3& n,
			const Vector2& ua, const Vector2& ub, const Vector2& uc, const Vector2& ud)
		{
			const auto base = static_cast<uint16_t>(verts.size());
			for (const auto& v : { a, b, c, d }) {
				verts.push_back(v);
				normals.push_back(n);
			}
			uvs.insert(uvs.end(), { ua, ub, uc, ud });
			tris.emplace_back(base, uint16_t(base + 1), uint16_t(base + 2));
			tris.emplace_back(base, uint16_t(base + 2), uint16_t(base + 3));
		}
	};

	int Check(const char* a_path)
	{
		NifFile nif;
		if (nif.Load(a_path) != 0 || !nif.IsValid()) {
			std::printf("FAILED to load %s\n", a_path);
			return 1;
		}
		const auto& version = nif.GetHeader().GetVersion();
		std::printf("version file=%u user=%u stream=%u\n", static_cast<unsigned>(version.File()), version.User(), version.Stream());
		std::printf("blocks=%u\n", nif.GetHeader().GetNumBlocks());
		for (uint32_t i = 0; i < nif.GetHeader().GetNumBlocks(); ++i) {
			std::printf("  %u %s\n", i, nif.GetHeader().GetBlockTypeStringById(i).c_str());
		}
		for (auto* shape : nif.GetShapes()) {
			const auto* verts = nif.GetVertsForShape(shape);
			std::printf("shape '%s' verts=%zu tris=%u\n", shape->name.get().c_str(), verts ? verts->size() : 0, shape->GetNumTriangles());
			std::string tex0, tex1;
			nif.GetTextureSlot(shape, tex0, 0);
			nif.GetTextureSlot(shape, tex1, 1);
			std::printf("  diffuse='%s' normal='%s'\n", tex0.c_str(), tex1.c_str());
			if (auto* shader = nif.GetShader(shape)) {
				std::printf("  shader=%s doubleSided=%d\n", shader->GetBlockName(), shader->IsDoubleSided() ? 1 : 0);
			}
			const auto bounds = shape->GetBounds();
			std::printf("  bounds center=(%.1f %.1f %.1f) radius=%.1f\n", bounds.center.x, bounds.center.y, bounds.center.z, bounds.radius);
		}
		return 0;
	}
}

int main(int argc, char** argv)
{
	if (argc == 3 && std::string(argv[1]) == "--check") {
		return Check(argv[2]);
	}
	if (argc != 2) {
		std::printf("usage: make_icon_nif <out.nif> | --check <in.nif>\n");
		return 2;
	}

	const float x0 = -W / 2, x1 = W / 2, z0 = 0.0f, z1 = H, yf = -D / 2, yb = D / 2;
	const Vector2 edge(EDGE_U, EDGE_V);

	Mesh m;
	// Front (faces -Y): viewer's right is +X; texture v=0 is the top.
	m.Quad({ x0, yf, z1 }, { x1, yf, z1 }, { x1, yf, z0 }, { x0, yf, z0 }, { 0, -1, 0 },
		{ 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 });
	// Back (faces +Y): viewer's right is -X, so u runs the other way.
	m.Quad({ x1, yb, z1 }, { x0, yb, z1 }, { x0, yb, z0 }, { x1, yb, z0 }, { 0, 1, 0 },
		{ 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 });
	// Edges: frame color.
	m.Quad({ x0, yb, z1 }, { x1, yb, z1 }, { x1, yf, z1 }, { x0, yf, z1 }, { 0, 0, 1 }, edge, edge, edge, edge);    // top
	m.Quad({ x0, yf, z0 }, { x1, yf, z0 }, { x1, yb, z0 }, { x0, yb, z0 }, { 0, 0, -1 }, edge, edge, edge, edge);   // bottom
	m.Quad({ x0, yb, z0 }, { x0, yb, z1 }, { x0, yf, z1 }, { x0, yf, z0 }, { -1, 0, 0 }, edge, edge, edge, edge);   // left
	m.Quad({ x1, yf, z0 }, { x1, yf, z1 }, { x1, yb, z1 }, { x1, yb, z0 }, { 1, 0, 0 }, edge, edge, edge, edge);    // right

	NifFile nif;
	auto version = NiVersion::getFO4();
	nif.Create(version);

	auto& hdr = nif.GetHeader();
	auto shape = std::make_unique<BSTriShape>();
	shape->Create(version, &m.verts, &m.tris, &m.uvs, &m.normals);
	shape->name.get() = "LostObjectsMenuIcon";

	auto textures = std::make_unique<BSShaderTextureSet>(version);
	textures->textures[0].get() = "textures\\LostObjects\\MenuIcon_d.dds";
	textures->textures[1].get() = "textures\\LostObjects\\MenuIcon_n.dds";

	auto shader = std::make_unique<BSLightingShaderProperty>(version);
	shader->TextureSetRef()->index = hdr.AddBlock(std::move(textures));
	shader->SetSkinned(false);
	shader->shaderFlags2 |= SLSF2_DOUBLE_SIDED;
	shader->specularStrength = 0.1f;

	shape->ShaderPropertyRef()->index = hdr.AddBlock(std::move(shader));
	const auto shapeID = hdr.AddBlock(std::move(shape));
	nif.GetRootNode()->childRefs.AddBlockRef(shapeID);

	if (nif.Save(argv[1]) != 0) {
		std::printf("FAILED to save %s\n", argv[1]);
		return 1;
	}
	std::printf("wrote %s\n", argv[1]);
	return Check(argv[1]);
}
