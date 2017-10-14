#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>

#include "CommonApp.h"

#include <stdio.h>

#include <DirectXMath.h>
using namespace DirectX;

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

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

	XMFLOAT3 getTriangleNormal(INT vertIndex);

	ID3D11Buffer *m_pHeightMapBuffer = nullptr;
	float m_rotationAngle;
	int m_HeightMapWidth;
	int m_HeightMapLength;
	int m_HeightMapVtxCount;
	XMFLOAT3* m_pHeightMap; // populated by LoadHeightMap();
	Vertex_Pos3fColour4ubNormal3f* m_pMapVtxs = nullptr;
	float m_cameraZ;
};

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

bool HeightMapApplication::HandleStart()
{
	this->SetWindowTitle("HeightMap");

	LoadHeightMap("HeightMap.bmp", 1.0f);

	m_cameraZ = 50.0f;

	m_pHeightMapBuffer = nullptr;

	m_rotationAngle = 0.f;

	if (!this->CommonApp::HandleStart())
	{
		return false;
	}

	static const VertexColour MAP_COLOUR(200, 255, 255, 255);

	const INT Width = 256;
	//const INT ArraySize = Width * Width;
	const INT ArraySize = (Width - 1) * Width * 2;

	m_HeightMapVtxCount = m_HeightMapWidth * m_HeightMapWidth * 6;
	m_pMapVtxs = new Vertex_Pos3fColour4ubNormal3f[m_HeightMapVtxCount];

	INT vertInd = 0;
	XMFLOAT3 normals[2];
	XMVECTOR tempVec;

	m_pMapVtxs[vertInd] = Vertex_Pos3fColour4ubNormal3f(m_pHeightMap[0], MAP_COLOUR, normals[0]);
	++vertInd;

	for (INT i = 0; i < Width - 1; ++i)
	{
		for (INT j = 0; j < Width - 1; ++j)
		{
			const INT idx = i + j * Width;

			if ((j == 0 && i > 0))
			{
				m_pMapVtxs[vertInd] = Vertex_Pos3fColour4ubNormal3f(m_pHeightMap[idx], MAP_COLOUR, normals[0]);
				m_pMapVtxs[vertInd + 1] = Vertex_Pos3fColour4ubNormal3f(m_pHeightMap[idx], MAP_COLOUR, normals[0]);
				vertInd += 2;
			}

			m_pMapVtxs[vertInd] = Vertex_Pos3fColour4ubNormal3f(m_pHeightMap[idx + Width], MAP_COLOUR, normals[0]);
			m_pMapVtxs[vertInd + 1] = Vertex_Pos3fColour4ubNormal3f(m_pHeightMap[idx + 1], MAP_COLOUR, normals[0]);
			vertInd += 2;

			if (j == Width - 2)
			{
				m_pMapVtxs[vertInd] = Vertex_Pos3fColour4ubNormal3f(m_pHeightMap[idx + Width + 1], MAP_COLOUR, normals[0]);
				m_pMapVtxs[vertInd + 1] = Vertex_Pos3fColour4ubNormal3f(m_pHeightMap[idx + Width + 1], MAP_COLOUR, normals[0]);
				vertInd += 2;
			}

		}
	}

	for (INT vertexIndex = 0; vertexIndex < ArraySize; vertexIndex += 2)
	{
		const XMFLOAT3 normal = getTriangleNormal(vertexIndex);

		m_pMapVtxs[vertexIndex].normal = *((D3DVECTOR*)&normal);
		m_pMapVtxs[vertexIndex + 1].normal = *((D3DVECTOR*)&normal);
		
		if (vertexIndex % 2 != 0)
		{

		}

	}

	m_pHeightMapBuffer = CreateImmutableVertexBuffer(m_pD3DDevice, sizeof Vertex_Pos3fColour4ubNormal3f * m_HeightMapVtxCount, m_pMapVtxs);

	delete m_pMapVtxs;

	return true;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void HeightMapApplication::HandleStop()
{
	Release(m_pHeightMapBuffer);

	this->CommonApp::HandleStop();
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void HeightMapApplication::HandleUpdate()
{
	m_rotationAngle += .01f;

	if (this->IsKeyPressed('Q'))
	{
		if (m_cameraZ > 20.0f)
			m_cameraZ -= 2.0f;
	}

	if (this->IsKeyPressed('A'))
	{
		m_cameraZ += 2.0f;
	}

}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

void HeightMapApplication::HandleRender()
{
	XMFLOAT3 vCamera(sin(m_rotationAngle)*m_cameraZ, m_cameraZ / 2, cos(m_rotationAngle)*m_cameraZ);
	XMFLOAT3 vLookat(0.0f, 0.0f, 0.0f);
	XMFLOAT3 vUpVector(0.0f, 1.0f, 0.0f);

	XMMATRIX  matView;
	matView = XMMatrixLookAtLH(XMLoadFloat3(&vCamera), XMLoadFloat3(&vLookat), XMLoadFloat3(&vUpVector));

	XMMATRIX matProj;
	matProj = XMMatrixPerspectiveFovLH(float(XM_PI / 4), 2, 1.5f, 5000.0f);

	this->SetViewMatrix(matView);
	this->SetProjectionMatrix(matProj);

	this->EnablePointLight(0, XMFLOAT3(100.0f, 100.f, -100.f), XMFLOAT3(1.f, 1.f, 1.f));

	this->Clear(XMFLOAT4(.2f, .2f, .6f, 1.f));


	//this->DrawUntexturedLit(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, m_pHeightMapBuffer, NULL, m_HeightMapVtxCount);
	static int s_count = 0;
	s_count += 100;
	if (s_count > m_HeightMapVtxCount)
	{
		s_count = 3;
	}

	//this->DrawUntexturedLit(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, m_pHeightMapBuffer, NULL, s_count);
	this->DrawUntexturedLit(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, m_pHeightMapBuffer, NULL, m_HeightMapVtxCount);
}

//////////////////////////////////////////////////////////////////////
// LoadHeightMap
// Original code sourced from rastertek.com
//////////////////////////////////////////////////////////////////////
bool HeightMapApplication::LoadHeightMap(char* filename, float gridSize)
{
	FILE* filePtr;
	int error;
	unsigned int count;
	BITMAPFILEHEADER bitmapFileHeader;
	BITMAPINFOHEADER bitmapInfoHeader;
	int imageSize, i, j, k, index;
	unsigned char* bitmapImage;
	unsigned char height;


	// Open the height map file in binary.
	error = fopen_s(&filePtr, filename, "rb");
	if (error != 0)
	{
		return false;
	}

	// Read in the file header.
	count = fread(&bitmapFileHeader, sizeof(BITMAPFILEHEADER), 1, filePtr);
	if (count != 1)
	{
		return false;
	}

	// Read in the bitmap info header.
	count = fread(&bitmapInfoHeader, sizeof(BITMAPINFOHEADER), 1, filePtr);
	if (count != 1)
	{
		return false;
	}

	// Save the dimensions of the terrain.
	m_HeightMapWidth = bitmapInfoHeader.biWidth;
	m_HeightMapLength = bitmapInfoHeader.biHeight;

	// Calculate the size of the bitmap image data.
	imageSize = m_HeightMapWidth * m_HeightMapLength * 3;

	// Allocate memory for the bitmap image data.
	bitmapImage = new unsigned char[imageSize];
	if (!bitmapImage)
	{
		return false;
	}

	// Move to the beginning of the bitmap data.
	fseek(filePtr, bitmapFileHeader.bfOffBits, SEEK_SET);

	// Read in the bitmap image data.
	count = fread(bitmapImage, 1, imageSize, filePtr);
	if (count != imageSize)
	{
		return false;
	}

	// Close the file.
	error = fclose(filePtr);
	if (error != 0)
	{
		return false;
	}

	// Create the structure to hold the height map data.
	m_pHeightMap = new XMFLOAT3[m_HeightMapWidth * m_HeightMapLength];
	if (!m_pHeightMap)
	{
		return false;
	}

	// Initialize the position in the image data buffer.
	k = 0;

	// Read the image data into the height map.
	for (j = 0; j < m_HeightMapLength; j++)
	{
		for (i = 0; i < m_HeightMapWidth; i++)
		{
			height = bitmapImage[k];

			index = (m_HeightMapLength * j) + i;

			m_pHeightMap[index].x = (float)(i - (m_HeightMapWidth / 2))*gridSize;
			m_pHeightMap[index].y = (float)height / 16 * gridSize;
			m_pHeightMap[index].z = (float)(j - (m_HeightMapLength / 2))*gridSize;

			k += 3;
		}
	}

	// Release the bitmap image data.
	delete[] bitmapImage;
	bitmapImage = 0;

	return true;
}

XMFLOAT3 HeightMapApplication::getTriangleNormal(INT vertIndex)
{
	//Vectors a & B are the vectors in the plane, vecN is the normal as a result of the cross product calculation 
	XMVECTOR vecA, vecB, vecN;
	XMFLOAT3 vecR; // resultant cross product

	vecA = XMLoadFloat3((XMFLOAT3*)&m_pMapVtxs[vertIndex + 1].pos) - XMLoadFloat3((XMFLOAT3*)&m_pMapVtxs[vertIndex].pos);
	vecB = XMLoadFloat3((XMFLOAT3*)&m_pMapVtxs[vertIndex + 2].pos) - XMLoadFloat3((XMFLOAT3*)&m_pMapVtxs[vertIndex].pos);

	vecN = XMVector3Cross(vecA, vecB);


	XMStoreFloat3(&vecR, vecN);
	return vecR;
}


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	HeightMapApplication application;

	Run(&application);

	return 0;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
