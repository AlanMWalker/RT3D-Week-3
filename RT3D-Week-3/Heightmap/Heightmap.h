#ifndef HEIGHTMAP_H
#define HEIGHTMAP_H

class HeightMapApplication :
	public CommonApp
{
public:
protected:
	bool HandleStart();
	void HandleStop();
	void HandleUpdate();
	void HandleRender();
	bool LoadHeightMap(char* filename, float gridSize);

private:

	XMFLOAT3 getTriangleNormal(INT vertIndex) const;
	XMFLOAT3 getVertexNormal(INT xPos, INT yPos) const;


	ID3D11Buffer *m_pHeightMapBuffer = nullptr;
	float m_rotationAngle;
	int m_HeightMapWidth;
	int m_HeightMapLength;
	int m_HeightMapVtxCount;
	XMFLOAT3* m_pHeightMap; // populated by LoadHeightMap();
	Vertex_Pos3fColour4ubNormal3f* m_pMapVtxs = nullptr;
	float m_cameraZ;
};


#endif
