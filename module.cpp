#include <stdio.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <boost/container/vector.hpp>
using boost::container::vector;
using namespace glm;
template<typename T> vector<T> &operator<<(vector<T> &a, T const& b) { a.push_back(b); return a; };
template<typename T> vector<T> &operator,(vector<T> &a, T const& b) { return a << b; };

typedef struct {
    vec3 min, max;
}AABB;

typedef struct {
    AABB box;
    int objectIndex;
    int parentIndex;
    int child1;
    int child2;
    bool isLeaf;
}Node;

typedef struct {
    Node* nodes;
    int nodeCount;
    int rootIndex;
}Tree;

AABB Union(AABB A, AABB B)
{
    return (AABB){ min(A.min, B.min), max(A.max, B.max), };
}

float Area(AABB A)
{
    vec3 d = A.max - A.min;
    return 2.0f * (d.x * d.y + d.y * d.z + d.z * d.x);
}

int AllocateInternalNode(Tree tree)
{
    return 0;
}

int AllocateLeafNode(Tree tree, int objectIndex, AABB box)
{
    return 0;
}

int PickBest(int bestSibling, int i)
{
    return 0;
}

void InsertLeaf(Tree tree, int objectIndex, AABB box)
{
    const int nullIndex = -1;

    int leafIndex = AllocateLeafNode(tree, objectIndex, box);
    if (tree.nodeCount == 0)
    {
        tree.rootIndex = leafIndex;
        return;
    }

    // Stage 1: find the best sibling for the new leaf
    int sibling = 0;
    for (int i = 0; i < tree.nodeCount; ++i)
    {
        sibling = PickBest(sibling, i);
    }

    // Stage 2: create a new parent
    int oldParent = tree.nodes[sibling].parentIndex;
    int newParent = AllocateInternalNode(tree);
    tree.nodes[newParent].parentIndex = oldParent;
    tree.nodes[newParent].box = Union(box, tree.nodes[sibling].box);
    if (oldParent != nullIndex)
    {
        // The sibling was not the root
        if (tree.nodes[oldParent].child1 == sibling)
        {
            tree.nodes[oldParent].child1 = newParent;
        }
        else
        {
            tree.nodes[oldParent].child2 = newParent;
        }
        tree.nodes[newParent].child1 = sibling;
        tree.nodes[newParent].child2 = leafIndex;
        tree.nodes[sibling].parentIndex = newParent;
        tree.nodes[leafIndex].parentIndex = newParent;
    }
    else
    {
        // The sibling was the root
        tree.nodes[newParent].child1 = sibling;
        tree.nodes[newParent].child2 = leafIndex;
        tree.nodes[sibling].parentIndex = newParent;
        tree.nodes[leafIndex].parentIndex = newParent;
        tree.rootIndex = newParent;
    }

    // Stage 3: walk back up the tree refitting AABBs
    int index = tree.nodes[leafIndex].parentIndex;
    while (index != nullIndex)
    {
        int child1 = tree.nodes[index].child1;
        int child2 = tree.nodes[index].child2;
        tree.nodes[index].box = Union(tree.nodes[child1].box, tree.nodes[child2].box);
        index = tree.nodes[index].parentIndex;
    }
}

int loadShader6(GLuint prog, const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "ERROR: file %s not found.", filename);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    rewind(f);
    char source[length+1]; source[length] = 0; // set null terminator
    fread(source, length, 1, f);
    fclose(f);

    { // detach shaders
        GLsizei numShaders;
        GLuint shaders[5];
        glGetAttachedShaders(prog, 5, &numShaders, shaders);
        for (int i=0; i<numShaders; i++) {
            glDetachShader(prog, shaders[i]);
        }
    }

    GLuint cs = glCreateShader(GL_COMPUTE_SHADER);
    const char *string[] = { source };
    glShaderSource(cs, 1, string, NULL);
    glCompileShader(cs);
    int success;
    glGetShaderiv(cs, GL_COMPILE_STATUS, &success);
    if (!success) {
        int length; glGetShaderiv(cs, GL_INFO_LOG_LENGTH, &length);
        char message[length]; glGetShaderInfoLog(cs, length, &length, message);
        fprintf(stderr, "ERROR: fail to compile compute shader. file \n%s\n", message);
        return 2;
    }
    glAttachShader(prog, cs);
    glDeleteShader(cs);
    glLinkProgram(prog);
    glValidateProgram(prog);
    return 0;
}

float sdBox( vec3 p, float b )
{
    vec3 q = abs(p) - b;
    return length(max(q, 0.f));
}

float sdTorus( vec3 p, vec2 t )
{
  vec2 q = vec2(length(vec2(p.x,p.z))-t.x,p.y);
  return length(q)-t.y;
}

float map(vec3 pos)
{
    float d1, d2;
    //d1 = sdBox(pos-vec3(0,1,0), .5) - .02;
    //d2 = length(pos-vec3(0,1,0)) - .5;
    //d1 = min(d1, d2);
    d2 = sdTorus(pos-vec3(0,1.,0), vec2(.5,.2));
    return d2;//max(d1, d2);
}

typedef struct { vec3 pos, nor; }Vertex;

typedef struct {
    GLuint count;
    GLuint instanceCount;
    GLuint firstIndex;
    GLint baseVertex;
    GLuint baseInstance;
}DrawElementsIndirectCommand;

static vector<Vertex> V;
static vector<GLushort> I;

int sdf2poly(uvec3 id, uint Res)
{
    const int Size = Res+1;
    static vector<GLushort> indexMap(Size*Size*Size);
    const float sca = .8*2/(Res);
    const vec3 offs = vec3(0,1,0) - .8f;

    static const vec3 vertmap[] = {
            {0,0,0}, {1,0,0}, {0,1,0}, {1,1,0},
            {0,0,1}, {1,0,1}, {0,1,1}, {1,1,1},
    };

    static const uvec2 edgevmap[] = {
            {0,1}, {2,3}, {4,5}, {6,7},
            {0,2}, {1,3}, {4,6}, {5,7},
            {0,4}, {1,5}, {2,6}, {3,7},
    };

    float d[8]; // density
    int mask = 0;
    for (int i=0; i<8; i++) {
        d[i] = map( (vec3(id)+vertmap[i])*sca + offs );
        mask |= (d[i] > 0) << i;
    }

    if (mask == 0 || mask == 0xff)
        return 0;

    int n_edges = 0;
    vec3 ce = vec3(0);
    for (int i=0; i<12; i++) {
        int  a = edgevmap[i][0];
        int  b = edgevmap[i][1];
        int m1 = (mask >> a) & 1;
        int m2 = (mask >> b) & 1;
        if (m1 ^ m2) {
            float t = d[a] / (d[a] - d[b]);
            assert(0 <= t && t <= 1);
            ce += mix(vertmap[a], vertmap[b], t);
            n_edges += 1;
        }
    }
    ce /= n_edges;

    int i0 = id.x + id.y*Size + id.z*Size*Size;
    indexMap[i0] = GLushort(V.size());
    V << Vertex{ offs + sca*(vec3(id)+ce), };

    if (id.x*id.y*id.z == 0)
        return 1;

    const int axismap[] = { 1, Size, Size*Size, 1, Size }; // wrap
    for (int i=0; i<3; i++) { // 3 basis axes
        const int lvl = 1 << i;
        const int m1 = mask & 1;
        const int m2 = (mask >> lvl) & 1;
        if (m1 ^ m2) { // compare to current cell with adjacent cell from each axes
            // flip face depends on the sign of density from lower end
            int i1 = i0 - axismap[i+1+m1],
                i2 = i0 - axismap[i+2-m1],
                i3 = i1 + i2 - i0;
            I <<
                indexMap[i0], indexMap[i1], indexMap[i2],
                indexMap[i2], indexMap[i1], indexMap[i3];
        }
    }

    return 2;
}

extern "C" int mainGeometry()
{
    static GLuint prog3, tex, frame = 0;
    if (!frame++) {
        prog3 = glCreateProgram();

        const int Size = 32;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_3D, tex);
        glTexStorage3D(GL_TEXTURE_3D, 1, GL_R8UI, Size,Size,Size);
        glBindTexture(GL_TEXTURE_3D, 0);
    }

    {
        assert(loadShader6(prog3, "../sdf.glsl") == 0);

        int data[128] = { 1 };
        GLuint ssbo;
        glGenBuffers(1, &ssbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof data, data, GL_STATIC_COPY);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

        glUseProgram(prog3);
        glDispatchCompute(2,2,2);
        //glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof data, data);
    }

    V.clear();
    I.clear();

    const int Res = 1<<5;
    for (int z=0; z<Res; z++)
        for (int y=0; y<Res; y++)
            for (int x=0; x<Res; x++)
                sdf2poly({x,y,z}, Res);

    // compute normals
    {
            uint firstIndex = 0;//cmd.back().firstIndex;
            int baseVertex = 0;//cmd.back().baseVertex;
            for (int i=baseVertex; i<V.size(); i++) {
                V[i].nor = vec3(0);
            }
            for (int i=firstIndex; i<I.size(); i+=3) {
                int i0 = I[i+0],
                    i1 = I[i+1],
                    i2 = I[i+2];
                vec3 p0 = V[baseVertex+i0].pos,
                     p1 = V[baseVertex+i1].pos,
                     p2 = V[baseVertex+i2].pos;
                vec3 nor = cross(p1-p0, p2-p1);
                V[baseVertex+i0].nor += nor;
                V[baseVertex+i1].nor += nor;
                V[baseVertex+i2].nor += nor;
            }
            for (int i=baseVertex; i<V.size(); i++) {
                V[i].nor = normalize(V[i].nor);
            }
    }

    // TODO: polygonize in octree

    vector<DrawElementsIndirectCommand> C = { {GLuint(I.size()),1,0,0,0} };

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 24, 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 24, (void*)12);
    glBufferData(GL_ARRAY_BUFFER, V.size()*sizeof V[0], V.data(), GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, I.size()*sizeof I[0], I.data(), GL_STATIC_DRAW);
    glBufferData(GL_DRAW_INDIRECT_BUFFER, C.size()*sizeof C[0], C.data(), GL_STATIC_DRAW);

    return C.size();
}

extern "C" int mainAnimation(float t)
{
    //t = M_PI;
    vec3 ta = vec3(0,1.3,0);
    vec3 ro = ta + vec3(sin(t),.3,cos(t))*2.5f;
    mat2x4 data = mat2x4(vec4(ro, 1.2), vec4(ta, 0));
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(vec4), sizeof(data), &data);
    return 0;
}
