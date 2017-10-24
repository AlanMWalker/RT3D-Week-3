#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>

#include "CommonApp.h"

#include <stdio.h>

#include <DirectXMath.h>
#include "Heightmap.h"

using namespace DirectX;

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

	m_HeightMapVtxCount = m_HeightMapWidth * m_HeightMapWidth * 6;
	m_pMapVtxs = new Vertex_Pos3fColour4ubNormal3f[m_HeightMapVtxCount];

	INT vertInd = 0;
	// --NORMALS

	XMFLOAT3* pNormals = nullptr;
	pNormals = new XMFLOAT3[m_HeightMapWidth *  m_HeightMapLength];

	for (INT i = 0; i < m_HeightMapWidth; ++i)
	{
		for (INT j = 0; j < m_HeightMapLength; ++j)
		{
			const INT index = j + i * m_HeightMapWidth;
			pNormals[index] = getVertexNormal(j, i);
		}
	}
	//-- NORMALS

	XMFLOAT3 normals[2];
	XMVECTOR tempVec;

	m_pMapVtxs[vertInd] = Vertex_Pos3fColour4ubNormal3f(m_pHeightMap[0], MAP_COLOUR, pNormals[0]);
	++vertInd;

	for (INT i = 0; i < m_HeightMapWidth - 1; ++i)
	{
		for (INT j = 0; j < m_HeightMapLength - 1; ++j)
		{
			if (i == 0 && j == 0)
			{
				continue;
			}
			const INT idx = j + i * m_HeightMapWidth;

			if (j == 0 && i > 0)
			{
				m_pMapVtxs[vertInd] = Vertex_Pos3fColour4ubNormal3f(m_pHeightMap[idx], MAP_COLOUR, pNormals[idx]);
				m_pMapVtxs[vertInd + 1] = Vertex_Pos3fColour4ubNormal3f(m_pHeightMap[idx], MAP_COLOUR, pNormals[idx]);
				vertInd += 2;
			}

			m_pMapVtxs[vertInd] = Vertex_Pos3fColour4ubNormal3f(m_pHeightMap[idx + Width], MAP_COLOUR, pNormals[idx + Width]);
			m_pMapVtxs[vertInd + 1] = Vertex_Pos3fColour4ubNormal3f(m_pHeightMap[idx + 1], MAP_COLOUR, pNormals[idx + 1]);
			vertInd += 2;

			if (j == Width - 2)
			{
				m_pMapVtxs[vertInd] = Vertex_Pos3fColour4ubNormal3f(m_pHeightMap[idx + Width], MAP_COLOUR, pNormals[idx + Width]);
				m_pMapVtxs[vertInd + 1] = Vertex_Pos3fColour4ubNormal3f(m_pHeightMap[idx + Width], MAP_COLOUR, pNormals[idx + Width]);
				vertInd += 2;
			}

		}
	}

	m_pHeightMapBuffer = CreateImmutableVertexBuffer(m_pD3DDevice, sizeof Vertex_Pos3fColour4ubNormal3f * m_HeightMapVtxCount, m_pMapVtxs);

	if (m_pMapVtxs)
	{
		delete m_pMapVtxs;
		m_pMapVtxs = nullptr;
	}

	if (pNormals)
	{
		delete[] pNormals;
		pNormals = nullptr;
	}

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

XMFLOAT3 HeightMapApplication::getTriangleNormal(INT vertIndex) const
{
	//Vectors a & B are the vectors in the plane, vecN is the normal as a result of the cross product calculation 
	XMVECTOR vecA, vecB, vecN;
	XMFLOAT3 vecR; // resultant cross product

	vecA = XMLoadFloat3(&m_pHeightMap[vertIndex + 1]) - XMLoadFloat3(&m_pHeightMap[vertIndex]);
	vecB = XMLoadFloat3(&m_pHeightMap[vertIndex + 2]) - XMLoadFloat3(&m_pHeightMap[vertIndex]);

	vecN = XMVector3Cross(vecA, vecB);

	XMStoreFloat3(&vecR, vecN);

	return vecR;
}

XMFLOAT3 HeightMapApplication::getVertexNormal(INT xPos, INT yPos) const
{
	XMFLOAT3 averagedFloat3(0.0f, 0.0f, 0.0f);
	XMVECTOR tempA, tempB;

	XMVECTOR* pVectors = nullptr;
	INT pVecSize = 0;

	auto toIndex = [](INT x, INT y, INT width)
	{
		return (x + y * width);
	};

	enum EGridPos
	{
		TopLeft,
		TopRight,
		BottomLeft,
		BottomRight,
		LeftBoundry,
		RightBoundry,
		TopBoundry,
		BottomBoundry,
		Central,
	};

	EGridPos gridPos = Central;
	//determine grid state
	if (xPos == 0 || xPos == m_HeightMapWidth - 1 || yPos == 0 || yPos == m_HeightMapWidth - 1)
	{

		const bool isTopLeft = (xPos == 0 && yPos == 0);
		const bool isTopRight = (xPos == m_HeightMapWidth - 1 && yPos == 0);
		const bool isBotLeft = (xPos == 0 && yPos == m_HeightMapLength - 1);
		const bool isBotRight = (xPos == m_HeightMapWidth - 1 && m_HeightMapLength - 1);

		if (isTopLeft)
		{
			gridPos = TopLeft;
		}
		else if (isTopRight)
		{
			gridPos = TopRight;

		}
		else if (isBotLeft)
		{
			gridPos = BottomLeft;
		}
		else if (isBotRight)
		{
			gridPos = BottomRight;
		}
		else
		{
			if (xPos == 0)
			{
				gridPos = LeftBoundry;
			}
			else if (xPos == m_HeightMapWidth - 1)
			{
				gridPos = RightBoundry;
			}
			else if (yPos == 0)
			{
				gridPos = TopBoundry;
			}
			else if (yPos == m_HeightMapLength - 1)
			{
				gridPos = BottomBoundry;
			}
		}
	}

	switch (gridPos)
	{

	case TopLeft:
	{
		pVecSize = 3;
		pVectors = new XMVECTOR[pVecSize];

		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos + 1, yPos, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[0] = tempA - tempB;


		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos + 1, yPos + 1, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[1] = tempA - tempB;


		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos + 1, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[2] = tempA - tempB;

	}
	break;

	case TopRight:
	{
		pVecSize = 3;
		pVectors = new XMVECTOR[pVecSize];

		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos - 1, yPos, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[0] = tempA - tempB;


		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos - 1, yPos + 1, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[1] = tempA - tempB;


		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos - 1, yPos - 1, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[2] = tempA - tempB;
	}
	break;

	case BottomLeft:
	{
		pVecSize = 3;
		pVectors = new XMVECTOR[pVecSize];

		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos + 1, yPos, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[0] = tempA - tempB;


		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos + 1, yPos - 1, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[1] = tempA - tempB;


		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos - 1, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[2] = tempA - tempB;
	}
	break;

	case BottomRight:
	{
		pVecSize = 3;
		pVectors = new XMVECTOR[pVecSize];

		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos - 1, yPos, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[0] = tempA - tempB;


		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos - 1, yPos - 1, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[1] = tempA - tempB;


		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos - 1, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[2] = tempA - tempB;
	}
	break;

	case LeftBoundry:
	{
		pVecSize = 5;
		pVectors = new XMVECTOR[pVecSize];

		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos - 1, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[0] = tempA - tempB;


		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos + 1, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[1] = tempA - tempB;


		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos + 1, yPos + 1, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[2] = tempA - tempB;

		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos + 1, yPos - 1, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[3] = tempA - tempB;

		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos + 1, yPos, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[4] = tempA - tempB;
	}
	break;

	case RightBoundry:
	{
		pVecSize = 5;
		pVectors = new XMVECTOR[pVecSize];

		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos - 1, yPos - 1, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[0] = tempA - tempB;


		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos - 1, yPos + 1, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[1] = tempA - tempB;


		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos + 1, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[2] = tempA - tempB;

		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos - 1, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[3] = tempA - tempB;

		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos - 1, yPos, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[4] = tempA - tempB;
	}
	break;

	case TopBoundry:
	{
		pVecSize = 5;
		pVectors = new XMVECTOR[pVecSize];

		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos - 1, yPos, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[0] = tempA - tempB;


		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos - 1, yPos + 1, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[1] = tempA - tempB;


		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos + 1, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[2] = tempA - tempB;

		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos + 1, yPos + 1, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[3] = tempA - tempB;

		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos + 1, yPos, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[4] = tempA - tempB;
	}
	break;

	case BottomBoundry:
	{
		pVecSize = 5;
		pVectors = new XMVECTOR[pVecSize];

		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos - 1, yPos, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[0] = tempA - tempB;


		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos + 1, yPos, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[1] = tempA - tempB;


		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos - 1, yPos - 1, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[2] = tempA - tempB;

		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos + 1, yPos - 1, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[3] = tempA - tempB;

		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos - 1, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[4] = tempA - tempB;
	}
	break;

	case Central:
	{
		pVecSize = 8;
		pVectors = new XMVECTOR[pVecSize];

		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos + 1, yPos, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[0] = tempA - tempB;

		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos + 1, yPos + 1, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[1] = tempA - tempB;

		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos + 1, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[2] = tempA - tempB;

		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos - 1, yPos, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[3] = tempA - tempB;

		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos - 1, yPos - 1, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[4] = tempA - tempB;

		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos - 1, yPos + 1, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[5] = tempA - tempB;

		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos + 1, yPos - 1, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[6] = tempA - tempB;

		tempA = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos - 1, m_HeightMapWidth)]);
		tempB = XMLoadFloat3(&m_pHeightMap[toIndex(xPos, yPos, m_HeightMapWidth)]);
		pVectors[7] = tempA - tempB;
	}
	break;

	default: break;

	}

	XMVECTOR averagedVec = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

	switch (pVecSize)
	{
	default:break;
	case 3:

		averagedVec += XMVector3Cross(pVectors[0], pVectors[1]);
		averagedVec += XMVector3Cross(pVectors[0], pVectors[2]);
		averagedVec /= 2.0f;
		break;
	case 5:

		averagedVec += XMVector3Cross(pVectors[0], pVectors[1]);
		averagedVec += XMVector3Cross(pVectors[2], pVectors[3]);
		averagedVec += XMVector3Cross(pVectors[0], pVectors[4]);
		//averagedVec += XMVector3Cross(pVectors[0], pVectors[4]);


		averagedVec /= 3.0f;

		break;
	case 8:
		for (int i = 0; i < pVecSize; ++i)
		{
			if (i + 1 < pVecSize)
			{
				averagedVec += XMVector3Cross(pVectors[i], pVectors[i + 1]);
			}
		}
		averagedVec /= 8.0f;
		break;
	}
	averagedVec = XMVector4Normalize(averagedVec);

	XMStoreFloat3(&averagedFloat3, averagedVec);
	averagedFloat3.y = fabs(averagedFloat3.y);

	delete[] pVectors;

	return averagedFloat3;
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
