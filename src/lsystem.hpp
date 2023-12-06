#ifndef LSYSTEM_HPP
#define LSYSTEM_HPP

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <glm/glm.hpp>
#include "gl_core_3_3.h"

class LSystem {
public:
	LSystem();
	~LSystem();
	// Move constructor and assignment
	LSystem(LSystem&& other);
	LSystem& operator=(LSystem&& other);
	// Disallow copy
	LSystem(const LSystem& other) = delete;
	LSystem& operator=(const LSystem& other) = delete;

	// Replace current L-system with the contents of the stream/string/file
	void parse(std::istream& istr);
	void parseString(std::string string);
	void parseFile(std::string filename);

	// Generate next iteration
	unsigned int iterate();

	// Draw the L-System
	void draw(glm::mat4 viewProj);
	void drawIter(unsigned int iter, glm::mat4 viewProj, float line_width);

	void update_time(float time);

	// Data access
	unsigned int getNumIter() const {
		return strings.size(); }
	std::string getString(unsigned int iter) const {
		return strings.at(iter); }

private:
	// Apply rules to a given string and return the result
	std::string applyRules(std::string string);
	// Create geometry for a given string and return the vertices
	std::vector<glm::vec3> createGeometry(std::string string);

	std::vector<std::string> strings;	// String representation of each iteration
	std::map<char, std::string> rules;	// Generation rules
	float angle;						// Angle for rotations

	// Holds geometry data about each iteration
	struct IterData {
		GLint first;		// Starting index in vertex buffer
		GLsizei count;		// Number of indices in iteration
		glm::mat4 bbfix;	// Scale and rotate to [-1,1], centered at origin
	};

	float cur_time;
	GLuint time_uniform_loc;
	float rot;
	float line_width;

	// OpenGL state
	static const GLsizei MAX_BUF = 1 << 26;		// Maximum buffer size
	GLuint vao;							// Vertex array object
	GLuint vbo;							// Vertex buffer
	std::vector<IterData> iterData;		// Iteration data
	GLsizei bufSize;					// Current size of the buffer
	void addVerts(std::vector<glm::vec3>& verts);	// Add iter geometry to buffer

	// Shared OpenGL state (shader)
	static unsigned int refcount;		// Reference counter
	static GLuint shader;				// Shader program
	static GLuint xformLoc;				// Location of matrix uniform
	void initShader();					// Create the shader program
};

#endif
