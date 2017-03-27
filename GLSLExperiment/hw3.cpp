#include "Angel.h"
#include "bmpread.h"

#define NUMPLYFILES 6
#define NUMARMPOINTS 22
#define NUMROOMPOINTS 18
static char plyFiles[NUMPLYFILES][20] = { "airplane.ply","ant.ply","apple.ply","cow.ply",
										"beethoven.ply","big_atc.ply" };

bmpread_t floorBitmap;
bmpread_t wallBitmap;
GLuint floorTexture;
GLuint wallTexture;
GLuint cubeTexture;

int numPlyVerts[NUMPLYFILES];
int numPlyPolygons[NUMPLYFILES];
float plyBoundingBoxes[NUMPLYFILES][3][2];

float curRotate0 = 0.0;
float curRotate1 = 0.0;
float curRotate2 = 0.0;
float curRotate3 = 0.0;
float curRotate4 = 0.0;
float curRotate5 = 0.0;
float bounceAmount = 0.0;
bool bounceState = false;
bool useFlatShading = false;
bool drawExtents = false;
bool drawReflections = false;
bool drawRefraction = false;
bool drawShadows = false;
bool drawTextures = false;

int width = 0;
int height = 0;

GLuint vertexBuffers[NUMPLYFILES];
GLuint normalBuffers[NUMPLYFILES];
GLuint roomBuffers[2]; // One for vertices, one for normals
GLuint armBuffers[2]; // One for vertices, one for normals

int numPoints = 0;
float light[4] = {0.0, 20.0, 0.0, 1.0};

GLuint program;

using namespace std;

class CTMNode{
    public:
    mat4 thisCTM;
    CTMNode * prevCTM;
    CTMNode() : prevCTM(NULL), thisCTM(Angel::identity()) {}
};

CTMNode curCTM;

void PushMatrix(){
    CTMNode * newCTM = new CTMNode;
    newCTM->thisCTM = mat4(curCTM.thisCTM);
    newCTM->prevCTM = curCTM.prevCTM;
    curCTM.prevCTM = newCTM; 
}

void PopMatrix(){
    if(curCTM.prevCTM == NULL){
        curCTM.thisCTM = Angel::identity();
    } else {
        curCTM = *(curCTM.prevCTM);
    }
}

void updateCTM(mat4 transform){
    curCTM.thisCTM = curCTM.thisCTM * transform;
    float ctmf[16];
    ctmf[0] = curCTM.thisCTM[0][0];ctmf[4] = curCTM.thisCTM[0][1];
	ctmf[1] = curCTM.thisCTM[1][0];ctmf[5] = curCTM.thisCTM[1][1];
	ctmf[2] = curCTM.thisCTM[2][0];ctmf[6] = curCTM.thisCTM[2][1];
	ctmf[3] = curCTM.thisCTM[3][0];ctmf[7] = curCTM.thisCTM[3][1];

	ctmf[8] = curCTM.thisCTM[0][2];ctmf[12] = curCTM.thisCTM[0][3];
	ctmf[9] = curCTM.thisCTM[1][2];ctmf[13] = curCTM.thisCTM[1][3];
	ctmf[10] = curCTM.thisCTM[2][2];ctmf[14] = curCTM.thisCTM[2][3];
	ctmf[11] = curCTM.thisCTM[3][2];ctmf[15] = curCTM.thisCTM[3][3];
    GLuint ctm = glGetUniformLocationARB(program, "ctm");
	glUniformMatrix4fv( ctm, 1, GL_FALSE, ctmf );
}

void resetPlyBox(int plyNum){
     plyBoundingBoxes[plyNum][0][0] = 9999999;
     plyBoundingBoxes[plyNum][0][1] = -9999999;
     plyBoundingBoxes[plyNum][1][0] = 9999999;
     plyBoundingBoxes[plyNum][1][1] = -9999999;
     plyBoundingBoxes[plyNum][2][0] = 9999999;
     plyBoundingBoxes[plyNum][2][1] = -9999999;
}

// Normal of next three vectors (contiguous from pointer)
vec4 calcNormal(vec4 * vectors){
    vec4 normal = vec4(0,0,0,0);
    for(int i=0; i<3; i+=1){
        vec4 curVec = vectors[i];
        vec4 nextVec = vectors[(i+1)%3];

        normal[0] = normal[0] + ((curVec[1] - nextVec[1])*(curVec[2] + nextVec[2]));
        normal[1] = normal[1] + ((curVec[2] - nextVec[2])*(curVec[0] + nextVec[0]));
        normal[2] = normal[2] + ((curVec[0] - nextVec[0])*(curVec[1] + nextVec[1]));
    }
    normal = normalize(normal);
    return normal;
}

vec4 roomVerts[NUMROOMPOINTS] = {
	// Floor
	vec4(-2,0,2,1),
	vec4(2,0,-2,1),
	vec4(-2,0,-2,1),
	vec4(2,0,-2,1),
	vec4(-2,0,2,1),
	vec4(2,0,2,1),

	// Right Wall
	vec4(-2,0,-2,1),
	vec4(2,0,-2,1),
	vec4(2,2,-2,1),
	vec4(-2,0,-2,1),
	vec4(2,2,-2,1),
	vec4(-2,2,-2,1),

	// Left Wall
	vec4(-2,0,2,1),
	vec4(-2,0,-2,1),
	vec4(-2,2,-2,1),
	vec4(-2,0,2,1),
	vec4(-2,2,-2,1),
	vec4(-2,2,2,1),
};

vec4 armVerts[NUMARMPOINTS] = {

    vec4( 0.0, -0.5, 0.0, 1.0 ),
    vec4( 0.0, -1.0, 0.0, 1.0 ),
    vec4( 0.0, -1.0, 0.0, 1.0 ),
    vec4( 1.5, -1.0, 0.0, 1.0 ),
    vec4( 1.5, -1.0, 0.0, 1.0 ),
    vec4( 1.5, -1.5, 0.0, 1.0 ),
    vec4( 0.0, -1.0, 0.0, 1.0 ),
    vec4(-1.5, -1.0, 0.0, 1.0 ),
    vec4(-1.5, -1.0, 0.0, 1.0 ),
    vec4(-1.5, -2.0, 0.0, 1.0 ),

    vec4(-1.5, -2., 0.0, 1.0 ),
    vec4(-1.5, -3.0, 0.0, 1.0 ),
    vec4(-1.5, -3.0, 0.0, 1.0 ),
    vec4(-0.75, -3.0, 0.0, 1.0 ),
    vec4(-1.5, -3.0, 0.0, 1.0 ),
    vec4(-2.25, -3.0, 0.0, 1.0 ),
    vec4(-0.75, -3.0, 0.0, 1.0 ),
    vec4(-0.75, -4.0, 0.0, 1.0 ),
    vec4(-2.25, -3.0, 0.0, 1.0 ),
    vec4(-2.25, -4.0, 0.0, 1.0 ),

	vec4( 1.5,-2.0,0.0,1.0 ),
	vec4( 1.5,-3.25,0.0,1.0 )
};

void checkSetBoundBox(int plyNum, vec4 point) {
	if (point[0]<plyBoundingBoxes[plyNum][0][0])
		plyBoundingBoxes[plyNum][0][0] = point[0];
	if (point[0]>plyBoundingBoxes[plyNum][0][1])
		plyBoundingBoxes[plyNum][0][1] = point[0];
	if (point[1]<plyBoundingBoxes[plyNum][1][0])
		plyBoundingBoxes[plyNum][1][0] = point[1];
	if (point[1]>plyBoundingBoxes[plyNum][1][1])
		plyBoundingBoxes[plyNum][1][1] = point[1];
	if (point[2]<plyBoundingBoxes[plyNum][2][0])
		plyBoundingBoxes[plyNum][2][0] = point[2];
	if (point[2]>plyBoundingBoxes[plyNum][2][1])
		plyBoundingBoxes[plyNum][2][1] = point[2];
}

void putAllPLYInBuffers() {
	for (int p = 0; p < NUMPLYFILES; p++) {
		int polyIndex = p;
		// Read curPly, put in buffer, and draw
		string file = "ply_files/" + string((char*)plyFiles[polyIndex]);
		fstream fs(file.c_str());
		string line;
		getline(fs, line);
		if (line != "ply\r")
			throw 20;
		getline(fs, line); // SKIP LINE 2
		getline(fs, line);
		sscanf(line.c_str(), "element vertex %d", &numPlyVerts[polyIndex]);
		getline(fs, line); // SKIP LINE
		getline(fs, line); // SKIP LINE
		getline(fs, line); // SKIP LINE
		getline(fs, line);
		sscanf(line.c_str(), "element face %d", &numPlyPolygons[polyIndex]);
		getline(fs, line); // SKIP LINE
		getline(fs, line); // SKIP LINE
		int curPlyVertices = numPlyVerts[polyIndex];
		int curPlyPolygons = numPlyPolygons[polyIndex];
		vec4 * verts = (vec4 *)malloc(sizeof(vec4) * numPlyVerts[polyIndex]);
		resetPlyBox(polyIndex);
		for (int i = 0; i < curPlyVertices; i++) {
			getline(fs, line);
			float x, y, z;
			sscanf(line.c_str(), "%f %f %f", &x, &y, &z);
			verts[i] = vec4(x, y, z, 1);
			checkSetBoundBox(polyIndex, verts[i]);
		}
		int * vertNums = (int *)malloc(sizeof(vec3) * curPlyPolygons * 3);
		vec4 * polygonVerts = (vec4 *)malloc(sizeof(vec4) * 3 * curPlyPolygons);
		vec4 * normals = (vec4 *)malloc(sizeof(vec4) * 3 * curPlyPolygons);
		vec4 * vertNormals = (vec4 *)malloc(sizeof(vec4) * curPlyVertices);
		for (int i = 0; i < curPlyVertices; i += 1)
            vertNormals[i] = vec4(0,0,0,0);
		for (int i = 0; i < curPlyPolygons; i += 1) {
			getline(fs, line);
			int p1, p2, p3;
			sscanf(line.c_str(), "3 %d %d %d", &p1, &p2, &p3);
			vertNums[(i * 3)] = p1;
			vertNums[(i * 3) + 1] = p2;
			vertNums[(i * 3) + 2] = p3;
			polygonVerts[(i * 3)] = verts[p1];
			polygonVerts[(i * 3) + 1] = verts[p2];
			polygonVerts[(i * 3) + 2] = verts[p3];
			vec4 normal = calcNormal(&polygonVerts[(i * 3)]);
			vertNormals[p1] += normal;
			vertNormals[p2] += normal;
			vertNormals[p3] += normal;
		}
		for (int i = 0; i < 3 * curPlyPolygons; i += 1) {
			normals[i] = normalize(vertNormals[vertNums[i]]);
		}
		vec4 * boundBox = (vec4*)malloc(sizeof(vec4) * 24);
		boundBox[0] = vec4(plyBoundingBoxes[polyIndex][0][1],plyBoundingBoxes[polyIndex][1][1],plyBoundingBoxes[polyIndex][2][1],1);
        boundBox[1] = vec4(plyBoundingBoxes[polyIndex][0][1],plyBoundingBoxes[polyIndex][1][1],plyBoundingBoxes[polyIndex][2][0],1);
        boundBox[2] = vec4(plyBoundingBoxes[polyIndex][0][1],plyBoundingBoxes[polyIndex][1][1],plyBoundingBoxes[polyIndex][2][0],1);
        boundBox[3] = vec4(plyBoundingBoxes[polyIndex][0][1],plyBoundingBoxes[polyIndex][1][0],plyBoundingBoxes[polyIndex][2][0],1);
        boundBox[4] = vec4(plyBoundingBoxes[polyIndex][0][1],plyBoundingBoxes[polyIndex][1][1],plyBoundingBoxes[polyIndex][2][0],1);
        boundBox[5] = vec4(plyBoundingBoxes[polyIndex][0][0],plyBoundingBoxes[polyIndex][1][1],plyBoundingBoxes[polyIndex][2][0],1);
        boundBox[6] = vec4(plyBoundingBoxes[polyIndex][0][0],plyBoundingBoxes[polyIndex][1][1],plyBoundingBoxes[polyIndex][2][0],1);
        boundBox[7] = vec4(plyBoundingBoxes[polyIndex][0][0],plyBoundingBoxes[polyIndex][1][0],plyBoundingBoxes[polyIndex][2][0],1);
        boundBox[8] = vec4(plyBoundingBoxes[polyIndex][0][0],plyBoundingBoxes[polyIndex][1][0],plyBoundingBoxes[polyIndex][2][0],1);
        boundBox[9] = vec4(plyBoundingBoxes[polyIndex][0][1],plyBoundingBoxes[polyIndex][1][0],plyBoundingBoxes[polyIndex][2][0],1);
        boundBox[10] = vec4(plyBoundingBoxes[polyIndex][0][1],plyBoundingBoxes[polyIndex][1][0],plyBoundingBoxes[polyIndex][2][0],1);
        boundBox[11] = vec4(plyBoundingBoxes[polyIndex][0][1],plyBoundingBoxes[polyIndex][1][0],plyBoundingBoxes[polyIndex][2][1],1);
        boundBox[12] = vec4(plyBoundingBoxes[polyIndex][0][1],plyBoundingBoxes[polyIndex][1][0],plyBoundingBoxes[polyIndex][2][1],1);
        boundBox[13] = vec4(plyBoundingBoxes[polyIndex][0][1],plyBoundingBoxes[polyIndex][1][1],plyBoundingBoxes[polyIndex][2][1],1);
        boundBox[14] = vec4(plyBoundingBoxes[polyIndex][0][1],plyBoundingBoxes[polyIndex][1][1],plyBoundingBoxes[polyIndex][2][1],1);
        boundBox[15] = vec4(plyBoundingBoxes[polyIndex][0][0],plyBoundingBoxes[polyIndex][1][1],plyBoundingBoxes[polyIndex][2][1],1);
        boundBox[16] = vec4(plyBoundingBoxes[polyIndex][0][0],plyBoundingBoxes[polyIndex][1][1],plyBoundingBoxes[polyIndex][2][1],1);
        boundBox[17] = vec4(plyBoundingBoxes[polyIndex][0][0],plyBoundingBoxes[polyIndex][1][1],plyBoundingBoxes[polyIndex][2][0],1);
        boundBox[18] = vec4(plyBoundingBoxes[polyIndex][0][0],plyBoundingBoxes[polyIndex][1][1],plyBoundingBoxes[polyIndex][2][1],1);
        boundBox[19] = vec4(plyBoundingBoxes[polyIndex][0][0],plyBoundingBoxes[polyIndex][1][0],plyBoundingBoxes[polyIndex][2][1],1);
        boundBox[20] = vec4(plyBoundingBoxes[polyIndex][0][0],plyBoundingBoxes[polyIndex][1][0],plyBoundingBoxes[polyIndex][2][1],1);
        boundBox[21] = vec4(plyBoundingBoxes[polyIndex][0][1],plyBoundingBoxes[polyIndex][1][0],plyBoundingBoxes[polyIndex][2][1],1);
        boundBox[22] = vec4(plyBoundingBoxes[polyIndex][0][0],plyBoundingBoxes[polyIndex][1][0],plyBoundingBoxes[polyIndex][2][1],1);
        boundBox[23] = vec4(plyBoundingBoxes[polyIndex][0][0],plyBoundingBoxes[polyIndex][1][0],plyBoundingBoxes[polyIndex][2][0],1);

		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[polyIndex]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec4) * ((3 * curPlyPolygons) + 24), NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec4) * 3 * curPlyPolygons, polygonVerts);
		glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec4) * 3 * curPlyPolygons, sizeof(vec4) * 24, boundBox);
        glBindBuffer(GL_ARRAY_BUFFER, normalBuffers[polyIndex]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec4) * ((3 * curPlyPolygons) + 24), NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec4) * 3 * curPlyPolygons, normals);
        glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec4) * 3 * curPlyPolygons, sizeof(vec4) * 24, boundBox);
		free(polygonVerts);
		free(normals);
		free(verts);
		free(vertNormals);
		free(vertNums);
	}
}

void putArmsInBuffers(){
    glBindBuffer(GL_ARRAY_BUFFER, armBuffers[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec4) * NUMARMPOINTS,armVerts,GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, armBuffers[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec4) * NUMARMPOINTS,armVerts,GL_STATIC_DRAW);
}

void putRoomInBuffers(){
	glBindBuffer(GL_ARRAY_BUFFER, roomBuffers[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec4)*NUMROOMPOINTS,roomVerts,GL_STATIC_DRAW);
	vec4 * normals = (vec4*)malloc(sizeof(vec4)*NUMROOMPOINTS);
	for(int i = 0; i < NUMROOMPOINTS; i+=3){
		vec4 thisNormal = calcNormal(&roomVerts[i]);
		normals[i] = thisNormal;
		normals[i+1] = thisNormal;
		normals[i+2] = thisNormal;
	}
	glBindBuffer(GL_ARRAY_BUFFER, roomBuffers[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec4)*NUMROOMPOINTS,normals,GL_STATIC_DRAW);
}

static void
myShadowMatrix(float ground[4], float light[4])
{
    float  dot;
    mat4  shadowMat;

    dot = ground[0] * light[0] +
          ground[1] * light[1] +
          ground[2] * light[2] +
          ground[3] * light[3];
    
    shadowMat[0][0] = dot - light[0] * ground[0];
    shadowMat[1][0] = 0.0 - light[0] * ground[1];
    shadowMat[2][0] = 0.0 - light[0] * ground[2];
    shadowMat[3][0] = 0.0 - light[0] * ground[3];
    
    shadowMat[0][1] = 0.0 - light[1] * ground[0];
    shadowMat[1][1] = dot - light[1] * ground[1];
    shadowMat[2][1] = 0.0 - light[1] * ground[2];
    shadowMat[3][1] = 0.0 - light[1] * ground[3];
    
    shadowMat[0][2] = 0.0 - light[2] * ground[0];
    shadowMat[1][2] = 0.0 - light[2] * ground[1];
    shadowMat[2][2] = dot - light[2] * ground[2];
    shadowMat[3][2] = 0.0 - light[2] * ground[3];
    
    shadowMat[0][3] = 0.0 - light[3] * ground[0];
    shadowMat[1][3] = 0.0 - light[3] * ground[1];
    shadowMat[2][3] = 0.0 - light[3] * ground[2];
    shadowMat[3][3] = dot - light[3] * ground[3];

    updateCTM(shadowMat);
}

void drawPlyFile(int plyNum, float x, float y, float z) {

	glActiveTexture( GL_TEXTURE1 );
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTexture);
	GLuint useReflection = glGetUniformLocationARB(program, "UseReflection");
	GLuint useRefraction = glGetUniformLocationARB(program, "UseRefraction");
	if(drawReflections)
		glUniform1f(useReflection, 1.0);
	if(drawRefraction)
		glUniform1f(useRefraction, 1.0);

	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[plyNum]);
	GLuint vPosition = glGetAttribLocation(program, "vPosition");
	glEnableVertexAttribArray(vPosition);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(0));

	glBindBuffer(GL_ARRAY_BUFFER, normalBuffers[plyNum]);
	GLuint vNormal = glGetAttribLocation(program, "vNormal");
	glEnableVertexAttribArray(vNormal);
	glVertexAttribPointer(vNormal, 4, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(0));

	vec3 polyCenter = vec3((plyBoundingBoxes[plyNum][0][0] + plyBoundingBoxes[plyNum][0][1]) / 2,
							(plyBoundingBoxes[plyNum][1][0] + plyBoundingBoxes[plyNum][1][1]) / 2,
							(plyBoundingBoxes[plyNum][2][0] + plyBoundingBoxes[plyNum][2][1]) / 2);

	vec3 polyScale = vec3(plyBoundingBoxes[plyNum][0][1] - plyBoundingBoxes[plyNum][0][0],
							plyBoundingBoxes[plyNum][1][1] - plyBoundingBoxes[plyNum][1][0],
							plyBoundingBoxes[plyNum][2][1] - plyBoundingBoxes[plyNum][2][0]);

	float maxScale = max(max(polyScale[0], polyScale[1]), polyScale[2]);
	mat4 scaleTransform;
    float nominalSize = 1.5;
	if (maxScale == polyScale[0]) {
		float yRatio = polyScale[1] / polyScale[0];
		float zRatio = polyScale[2] / polyScale[0];
		scaleTransform = Scale(nominalSize / polyScale[0], (yRatio * nominalSize) / polyScale[1], (zRatio * nominalSize) / polyScale[2]);
	}
	else if (maxScale == polyScale[1]) {
		float xRatio = polyScale[0] / polyScale[1];
		float zRatio = polyScale[2] / polyScale[1];
		scaleTransform = Scale((xRatio * nominalSize) / polyScale[0], nominalSize / polyScale[1], (zRatio * nominalSize) / polyScale[2]);
	}
	else if (maxScale == polyScale[2]) {
		float xRatio = polyScale[0] / polyScale[2];
		float yRatio = polyScale[1] / polyScale[2];
		scaleTransform = Scale((xRatio * nominalSize) / polyScale[0], (yRatio * nominalSize) / polyScale[1], nominalSize / polyScale[2]);
	}
	PushMatrix();
	updateCTM(Translate(x, y, z));
	updateCTM(scaleTransform);
	updateCTM(Translate(-polyCenter[0], -polyCenter[1], -polyCenter[2]));
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_DEPTH_TEST);
	glDrawArrays(GL_TRIANGLES, 0, 3 * numPlyPolygons[plyNum]);
    if(drawExtents){
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDrawArrays(GL_LINES, 3*numPlyPolygons[plyNum], 24);
    }
	glDisable(GL_DEPTH_TEST);
	glUniform1f(useReflection, 0.0);
	glUniform1f(useRefraction, 0.0);
	PopMatrix();
	
	if(!drawShadows)
		return;
	// Floor Shadow
	PushMatrix();
	//updateCTM(Translate(-light[0],-light[1],-light[2]));
	updateCTM(Translate(x,0,z));
	updateCTM(Translate(0,-5.0,0.0));
	updateCTM(Scale(1.0,0.0,1.0));
	//updateCTM(Translate(0,0,-light[2]));
	// updateCTM(shadowMat);
	// updateCTM(Translate(light[0],light[1],light[2]));
	updateCTM(scaleTransform);
	updateCTM(Translate(-polyCenter[0], -polyCenter[1], -polyCenter[2]));
	//updateCTM(Translate(light[0],light[1],light[2]));
	GLuint ambient = glGetUniformLocationARB(program, "AmbientProduct");
	glUniform4fv( ambient, 1, vec4(0.2,0.2,0.2,0.2));
    GLuint diffuse = glGetUniformLocationARB(program, "DiffuseProduct");
	glUniform4fv( diffuse, 1, vec4(0,0,0,1));
    GLuint specular = glGetUniformLocationARB(program, "SpecularProduct");
	glUniform4fv( specular, 1, vec4(0,0,0,1));
	glEnable(GL_DEPTH_TEST);
	glDrawArrays(GL_TRIANGLES, 0, 3 * numPlyPolygons[plyNum]);
	glDisable(GL_DEPTH_TEST);
	PopMatrix();
}

void drawArm(int armNum){
    glBindBuffer(GL_ARRAY_BUFFER, armBuffers[0]);
	GLuint vPosition = glGetAttribLocation(program, "vPosition");
	glEnableVertexAttribArray(vPosition);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(0));

	glBindBuffer(GL_ARRAY_BUFFER, armBuffers[1]);
	GLuint vNormal = glGetAttribLocation(program, "vNormal");
	glEnableVertexAttribArray(vNormal);
	glVertexAttribPointer(vNormal, 4, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(0));
    
    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    glEnable( GL_DEPTH_TEST );
    if(armNum == 2)
        glDrawArrays(GL_LINES, 20, 2);
    else
        glDrawArrays(GL_LINES, 10 * armNum, 10);
    glDisable( GL_DEPTH_TEST );
}

void drawFloor( void )
{
	glBindBuffer(GL_ARRAY_BUFFER, roomBuffers[0]);
	GLuint vPosition = glGetAttribLocation(program, "vPosition");
	glEnableVertexAttribArray(vPosition);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(0));

	glBindBuffer(GL_ARRAY_BUFFER, roomBuffers[1]);
	GLuint vNormal = glGetAttribLocation(program, "vNormal");
	glEnableVertexAttribArray(vNormal);
	glVertexAttribPointer(vNormal, 4, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(0));

	glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, floorTexture);

	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    glEnable( GL_DEPTH_TEST );
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisable( GL_DEPTH_TEST );
}

void drawWalls( void )
{
	glActiveTexture( GL_TEXTURE0 );
    glBindTexture(GL_TEXTURE_2D, wallTexture);
	// Assume drawFloor called last -> pointer already in roombuffer
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    glEnable( GL_DEPTH_TEST );
    glDrawArrays(GL_TRIANGLES, 6, 12);
    glDisable( GL_DEPTH_TEST );
}

void generateGeometry( void )
{	
	cout << "Loading grass.bmp" << endl;
	if(!bmpread("grass.bmp", 0, &floorBitmap)){
		cout << "error:could not read grass.bmp" << endl;
		exit(1);
	}
	cout << "Loading stones.bmp" << endl;
	if(!bmpread("stones.bmp", 0, &wallBitmap)){
		cout << "error:could not read stones.bmp" << endl;
		exit(1);
	}

	glActiveTexture( GL_TEXTURE0 );
	glGenTextures( 1, &floorTexture);
	glGenTextures( 1, &wallTexture);
	glBindTexture( GL_TEXTURE_2D, floorTexture);
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, floorBitmap.width, floorBitmap.height, 0, GL_RGB, GL_UNSIGNED_BYTE, floorBitmap.rgb_data );
	glBindTexture( GL_TEXTURE_2D, wallTexture);
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, wallBitmap.width, wallBitmap.height, 0, GL_RGB, GL_UNSIGNED_BYTE, wallBitmap.rgb_data );
	bmpread_free(&floorBitmap);
	bmpread_free(&wallBitmap);

	bmpread_t nvnegx;
	bmpread_t nvnegy;
	bmpread_t nvnegz;
	bmpread_t nvposx;
	bmpread_t nvposy;
	bmpread_t nvposz;

	cout << "Loading nvnegx.bmp" << endl;
	if(!bmpread("nvnegx.bmp", 0, &nvnegx)){
		cout << "error:could not read nvnegx.bmp" << endl;
		exit(1);
	}
	cout << "Loading nvnegy.bmp" << endl;
	if(!bmpread("nvnegy.bmp", 0, &nvnegy)){
		cout << "error:could not read nvnegy.bmp" << endl;
		exit(1);
	}
	cout << "Loading nvnegz.bmp" << endl;
	if(!bmpread("nvnegz.bmp", 0, &nvnegz)){
		cout << "error:could not read nvnegz.bmp" << endl;
		exit(1);
	}
	cout << "Loading nvposx.bmp" << endl;
	if(!bmpread("nvposx.bmp", 0, &nvposx)){
		cout << "error:could not read nvposx.bmp" << endl;
		exit(1);
	}
	cout << "Loading nvposy.bmp" << endl;
	if(!bmpread("nvposy.bmp", 0, &nvposy)){
		cout << "error:could not read nvposy.bmp" << endl;
		exit(1);
	}
	cout << "Loading nvposz.bmp" << endl;
	if(!bmpread("nvposz.bmp", 0, &nvposz)){
		cout << "error:could not read nvposz.bmp" << endl;
		exit(1);
	}

	// Cube Map
	glActiveTexture( GL_TEXTURE1 );
	glEnable(GL_TEXTURE_CUBE_MAP);
	glGenTextures(1,&cubeTexture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTexture);

	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X,0,GL_RGB,nvposx.width, nvposx.height, 0, GL_RGB, GL_UNSIGNED_BYTE, nvposx.rgb_data );
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X,0,GL_RGB,nvnegx.width, nvnegx.height, 0, GL_RGB, GL_UNSIGNED_BYTE, nvnegx.rgb_data );
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y,0,GL_RGB,nvposy.width, nvposy.height, 0, GL_RGB, GL_UNSIGNED_BYTE, nvposy.rgb_data );
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,0,GL_RGB,nvnegy.width, nvnegy.height, 0, GL_RGB, GL_UNSIGNED_BYTE, nvnegy.rgb_data );
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z,0,GL_RGB,nvposz.width, nvposz.height, 0, GL_RGB, GL_UNSIGNED_BYTE, nvposz.rgb_data );
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,0,GL_RGB,nvnegz.width, nvnegz.height, 0, GL_RGB, GL_UNSIGNED_BYTE, nvnegz.rgb_data );
	glTexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	bmpread_free(&nvnegx);
	bmpread_free(&nvnegy);
	bmpread_free(&nvnegz);
	bmpread_free(&nvposx);
	bmpread_free(&nvposy);
	bmpread_free(&nvposz);

    // Create a vertex array object
    GLuint vao;
    glGenVertexArrays( 1, &vao );
    glBindVertexArray( vao );

	// Load shaders and use the resulting shader program
    program = InitShader( "vshader1.glsl", "fshader1.glsl" );
    glUseProgram( program );
     // set up vertex arrays
    glGenBuffers( NUMPLYFILES, vertexBuffers );
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[0]);
	GLuint vPosition = glGetAttribLocation(program, "vPosition");
	glEnableVertexAttribArray(vPosition);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(0));

	GLuint vTexCoord = glGetAttribLocation( program, "vTexCoord" ); 
    glEnableVertexAttribArray( vTexCoord );
    glVertexAttribPointer( vTexCoord, 4, GL_FLOAT, GL_FALSE, 0,
			   BUFFER_OFFSET(0) );

    glGenBuffers( NUMPLYFILES, normalBuffers );
	glBindBuffer(GL_ARRAY_BUFFER, normalBuffers[0]);
	GLuint vNormal = glGetAttribLocation(program, "vNormal");
	glEnableVertexAttribArray(vNormal);
	glVertexAttribPointer(vNormal, 4, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(0));

    glGenBuffers( 2, armBuffers );
	glGenBuffers( 2, roomBuffers );

	GLuint texMapLocation;
	texMapLocation = glGetUniformLocationARB(program,"texMap");
	glUniform1i(texMapLocation, 1);

    //loadGeometry();
	putAllPLYInBuffers();
    putArmsInBuffers();
	putRoomInBuffers();

	// sets the default color to clear screen
    glClearColor( 0.0, 0.0, 0.0, 1.0 ); // black background
}

// Calculate the center of the next three vectors (contiguous from pointer)
vec4 calcCenter(vec4 * vectors){
    vec4 center = vec4(0,0,0,1);
    center[0] = (vectors[0][0] + vectors[1][0] + vectors[2][0])/3;
    center[1] = (vectors[0][1] + vectors[1][1] + vectors[2][1])/3;
    center[2] = (vectors[0][2] + vectors[1][2] + vectors[2][2])/3;
    return center;
}

void display( void )
{
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );     // clear the window
    GLuint lightPos = glGetUniformLocationARB(program, "LightPosition");
	glUniform4fv( lightPos, 1, vec4(0.0,10.0,6.0,1.0));
    // Lighting Stuff
    GLuint ambient = glGetUniformLocationARB(program, "AmbientProduct");
	glUniform4fv( ambient, 1, vec4(0.2,0,0,1));
    GLuint diffuse = glGetUniformLocationARB(program, "DiffuseProduct");
	glUniform4fv( diffuse, 1, vec4(0.4,0,0,1));
    GLuint specular = glGetUniformLocationARB(program, "SpecularProduct");
	glUniform4fv( specular, 1, vec4(0.4,0,0,1));
    GLuint shininess = glGetUniformLocationARB(program, "Shininess");
    glUniform1f( shininess, 32.0f);
	GLuint flatShading = glGetUniformLocationARB(program, "UseFlatShading");
	glUniform1f(flatShading, useFlatShading);
	GLuint useTexture = glGetUniformLocationARB(program, "UseTexture");
	glUniform1f(useTexture, drawTextures);
	GLuint useReflection = glGetUniformLocationARB(program, "UseReflection");
	glUniform1f(useReflection, 0.0);
	GLuint iorefr = glGetUniformLocationARB(program, "iorefr");
	glUniform1f(iorefr, 1.2);

    curCTM = CTMNode();
    updateCTM(Perspective((GLfloat)60.0, (GLfloat)width/(GLfloat)height, (GLfloat)0.1, (GLfloat) 200.0));
	updateCTM(Translate(0,2.2,-6));
	PushMatrix();
	// Draw Floor and Walls
	updateCTM(Translate(0,-5,0));
	updateCTM(Scale(2,6,2));
	updateCTM(RotateY(-45));
	glUniform4fv(ambient, 1, vec4(0.2, 0.2, 0.2, 1));
	glUniform4fv(diffuse, 1, vec4(0.4,0.4, 0.4, 1));
	glUniform4fv(specular, 1, vec4(0.4, 0.4, 0.4, 1));
	glUniform1f(useTexture, drawTextures);
	drawFloor();
	drawWalls();
	glUniform1f(useTexture, 0.0);
	PopMatrix();
    updateCTM(RotateY(curRotate0));
    updateCTM(Translate(0, .08*(sin(bounceAmount) + 1),0));
	glUniform4fv(ambient, 1, vec4(0.2, 0, 0, 1));
	glUniform4fv(diffuse, 1, vec4(0.4,0, 0, 1));
	glUniform4fv(specular, 1, vec4(0.4, 0, 0, 1));
    drawArm(0);
    drawPlyFile(0,0,0,0);
    PushMatrix();
    updateCTM(Translate(-1.5,0,0));
    updateCTM(RotateY(curRotate1));
    updateCTM(Translate(1.5,0,0));
    updateCTM(Translate(0,.08*(cos(bounceAmount) + 1),0));
	glUniform4fv(ambient, 1, vec4(0, 0.2, 0, 1));
	glUniform4fv(diffuse, 1, vec4(0,0.4, 0, 1));
	glUniform4fv(specular, 1, vec4(0, 0.4, 0, 1));
    drawArm(1);
    drawPlyFile(1,-1.5,-2.0,0);
	PushMatrix();
    updateCTM(Translate(-0.75,0,0));
    updateCTM(RotateY(curRotate3));
    updateCTM(Translate(0.75,0,0));
    updateCTM(Translate(0,.08*(sin(bounceAmount) + 1),0));
	glUniform4fv(ambient, 1, vec4(0, 0, 0.2, 1));
	glUniform4fv(diffuse, 1, vec4(0, 0, 0.4, 1));
	glUniform4fv(specular, 1, vec4(0, 0, 0.4, 1));
    drawPlyFile(3,-0.75,-4.0,0);
    PopMatrix();
    PushMatrix();
    updateCTM(Translate(-2.25,0,0));
    updateCTM(RotateY(curRotate4));
    updateCTM(Translate(2.25,0,0));
    updateCTM(Translate(0,.08*(sin(bounceAmount) + 1),0));
	glUniform4fv(ambient, 1, vec4(0.2, 0.2, 0.2, 1));
	glUniform4fv(diffuse, 1, vec4(0.4, 0.4, 0.4, 1));
	glUniform4fv(specular, 1, vec4(0.4, 0.4, 0.4, 1));
	drawPlyFile(4, -2.25, -4.0, 0);
    PopMatrix();
    PopMatrix();
    PushMatrix();
    updateCTM(Translate(1.5,0,0));
    updateCTM(RotateY(curRotate2));
    updateCTM(Translate(-1.5,0,0));
    updateCTM(Translate(0,.08*(cos(bounceAmount) + 1),0));
	glUniform4fv(ambient, 1, vec4(0.2, 0, 0.2, 1));
	glUniform4fv(diffuse, 1, vec4(0.4, 0, 0.4, 1));
	glUniform4fv(specular, 1, vec4(0.4, 0, 0.4, 1));
	drawPlyFile(2, 1.5, -2.0, 0);
	PushMatrix();
	updateCTM(Translate(1.5, 0, 0));
	updateCTM(RotateY(curRotate5));
	updateCTM(Translate(-1.5, 0, 0));
	updateCTM(Translate(0, .08*(sin(bounceAmount) + 1), 0));
	glUniform4fv(ambient, 1, vec4(0.2, 0.2, 0.0, 1));
	glUniform4fv(diffuse, 1, vec4(0.4, 0.4, 0.0, 1));
	glUniform4fv(specular, 1, vec4(0.4, 0.4, 0.0, 1));
    drawArm(2);
    drawPlyFile(5,1.5,-3.5,0);
    PopMatrix();
    glFlush(); // force output to graphics hardware
	// use this call to double buffer
	glutSwapBuffers();
}

//----------------------------------------------------------------------------
//keyboard handler
void keyboard( unsigned char key, int x, int y )
{
    switch ( key ) {
		case 'e':
			drawExtents = !drawExtents;
			break;
        case 'm':
			useFlatShading = false;
            break;
        case 'M':
			useFlatShading = true;
            break;
		case 'C':
			drawRefraction = false;
			drawReflections = !drawReflections;
			break;
		case 'D':
			drawReflections = false;
			drawRefraction = !drawRefraction;
			break;
		case 'A':
			drawShadows = !drawShadows;
			break;
		case 'B':
			drawTextures = !drawTextures;
			break;
        default:
            break;
    }
}


void animFunc( int val ){
    curRotate0+=0.2;
    curRotate1-=0.4;
    curRotate2-=0.5;
    curRotate3+=0.25;
    curRotate4+=0.66;
	curRotate5 += 0.53;
    if(bounceState)
        bounceAmount += 0.1;
    glutPostRedisplay();
    glutTimerFunc(30, animFunc, 0);
}

//----------------------------------------------------------------------------
int main( int argc, char **argv )
{
    glutInit( &argc, argv );
    glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH );
    glutInitWindowSize( 768, 768);
	width = 768;
	height = 768;

    glutCreateWindow( "Homework 3: Andrew McAfee" );

	glewExperimental = 1;
    glewInit();

    generateGeometry();

    glutDisplayFunc( display );
    glutKeyboardFunc( keyboard );
    glutTimerFunc(30, animFunc, 0);

    glutMainLoop();
    return 0;
}
