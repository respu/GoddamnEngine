#ifndef GD_RESOURCE_STATIC_MESH_STATIC_MESH
#define GD_RESOURCE_STATIC_MESH_STATIC_MESH

#include <GoddamnEngine/Include.hh>
#include <GoddamnEngine/Engine/Resource/Resource.hh>

#define GD_STATIC_MESH_MAKE_WORD(hi, lo)	((ushort)(((byte)lo) | ((byte)hi << 8)))
#define GD_STATIC_MESH_EXTRACT_LO(word)		((byte)(word))
#define GD_STATIC_MESH_EXTRACT_HI(word)		((byte)((word & 0xFF00) >> 8))

GD_NAMESPACE_BEGIN

	class HRIBuffer;
	class HRIIndexedShape;

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	/// Enumeration describing versions of Shekel(C) model version
	enum class ShekelVersion : UInt16
	{
		Version01 = GD_STATIC_MESH_MAKE_WORD(0, 1),			///< Version 0.1
		Version10 = GD_STATIC_MESH_MAKE_WORD(1, 0),			///< Version 1.0
		Unknown = 0											///< Unknown version.
	};

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	/// Loads supported 3D-model formats into engine and
	///	manages different levels of details	
	class StaticMesh final : public Resource
	{
	private:
		GD_TYPEINFORMATION_DEFINITION(StaticMesh, Resource, GDAPI);

		Vector<HRIIndexedShape*> levelsOfDetails;
		size_t bindedLevelOfDetails;

		GDAPI StaticMesh(String const& identifier);

		GDAPI virtual ~StaticMesh();

	public:
		//////////////////////////////////////////////////////////////////////////
		/// Returns number of LODs mesh contains
		GDAPI size_t GetNumberOfLods() const;

		//////////////////////////////////////////////////////////////////////////
		/// Changes number of LODs mesh contains
		GDAPI void SetNumberOfLods(size_t const numberOfLods);

		//////////////////////////////////////////////////////////////////////////
		/// Destroys LOD at specified level
		GDAPI void DestroyLodAt(size_t const lod);

		//////////////////////////////////////////////////////////////////////////
		/// Returns LOD containing in mesh
		GDAPI const HRIIndexedShape* GetLodAt(size_t const lod) const;
		GDAPI       HRIIndexedShape* GetLodAt(size_t const lod)      ;

		//////////////////////////////////////////////////////////////////////////
		/// Copies data to LOD at specified level. LOD at level may not exist.
		GDAPI void SetLodAt(size_t const lod, HRIIndexedShape const* const submesh);

		//////////////////////////////////////////////////////////////////////////
		/// Switches to LOD that would be used while binding
		GDAPI void SwitchToLod(size_t const lod);

		//////////////////////////////////////////////////////////////////////////
		/// Checks if two meshes have similar buffers.
		GDAPI bool CompareSignature(StaticMesh const* const staticMesh) const;

		//////////////////////////////////////////////////////////////////////////
		/// Merges two meshes
		GDAPI void CombineWithMesh(StaticMesh const* staticMesh);

	private:
		GDAPI virtual void OnResourceLoad(ResourceLoader* const resourceLoader);
		GDAPI virtual void OnResourceUnload();

	};

GD_NAMESPACE_END

#endif