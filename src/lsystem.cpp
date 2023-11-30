#define NOMINMAX
#include "lsystem.hpp"
#include <fstream>
#include <sstream>
#include <stack>
#include <glm/gtc/type_ptr.hpp>
#include <math.h>
#include "util.hpp"

// Stream processing helper functions
std::stringstream preprocessStream(std::istream& istr);
std::string getNextLine(std::istream& istr);
std::string trim(const std::string& line);

// Static L-System members
unsigned int LSystem::refcount = 0;
GLuint LSystem::shader = 0;
GLuint LSystem::xformLoc = 0;

// Constructor
LSystem::LSystem() :
	angle(0.0f),
	vao(0),
	vbo(0),
	bufSize(0) {

	// Create shader if we're the first object
	if (refcount == 0)
		initShader();
	refcount++;
}

// Destructor
LSystem::~LSystem() {
	// Destroy vertex buffer and array
	if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
	if (vbo) { glDeleteBuffers(1, &vbo); vbo = 0; }
	bufSize = 0;

	refcount--;
	// Destroy shader if we're the last object
	if (refcount == 0) {
		if (shader) { glDeleteProgram(shader); shader = 0; }
	}
}

// Move constructor
LSystem::LSystem(LSystem&& other) :
	strings(std::move(other.strings)),
	rules(std::move(other.rules)),
	angle(other.angle),
	vao(other.vao),
	vbo(other.vbo),
	iterData(std::move(other.iterData)),
	bufSize(other.bufSize) {

	other.vao = 0;
	other.vbo = 0;
	other.bufSize = 0;
	// Increment reference count (temp will decrement upon destructor)
	refcount++;
}

// Move assignment operator
LSystem& LSystem::operator=(LSystem&& other) {
	strings = std::move(other.strings);
	rules = std::move(other.rules);
	angle = other.angle;
	iterData = std::move(other.iterData);
	bufSize = other.bufSize;

	// Release any existing buffers
	if (vao) { glDeleteVertexArrays(1, &vao); }
	if (vbo) { glDeleteBuffers(1, &vbo); }
	// Acquire other's buffers
	vao = other.vao;
	vbo = other.vbo;

	other.vao = 0;
	other.vbo = 0;
	other.bufSize = 0;
	// Refcount stays the same

	return *this;
}

// Parse input stream and replace current L-System with contents
// Assumes valid input has no comments and ends with a newline character
void LSystem::parse(std::istream& istr) {
	// Temporary storage as input stream is parsed
	float inAngle = 0.0f;
	unsigned int inIters = 0;
	std::string inAxiom;
	std::map<char, std::string> inRules;


	// TODO: ==================================================================
	// Parse the input stream
	// Store the rotation angle in "inAngle"
	// Store the number of iterations in "inIters"
	// Store the axiom in "inAxiom"
	// Store the set of rules in "inRules"

	// Remove this line
	//throw std::runtime_error("Parser not implemented");
	std:: string tmp;
	char key;
	istr>>tmp;
	inAngle = std::stof(tmp);
	istr>>tmp;
	inIters = std::stoi(tmp);
	istr>>tmp;
	inAxiom = tmp;
	
	while(istr>>tmp){
		key = tmp[0];
		istr>>tmp;
		istr>>tmp;
		inRules[key] = tmp;
	}

	// Make your changes above this line
	// END TODO ===============================================================


	// Replace current state with parsed contents
	angle = inAngle;
	strings = { inAxiom };
	rules = std::move(inRules);
	// Create geometry for axiom
	iterData.clear();
	auto verts = createGeometry(strings.back());
	addVerts(verts);

	// Perform iterations
	try {
		while (strings.size() < inIters)
			iterate();
	} catch (const std::exception& e) {
		// Failed to iterate, stop at last iter
		std::cerr << "Too many iterations: geometry exceeds maximum buffer size" << std::endl;
	}
}

// Parse contents of source string
void LSystem::parseString(std::string string) {
	std::stringstream ss(string);

	// Preprocess to remove comments & whitespace
	ss = preprocessStream(ss);
	parse(ss);
}

// Parse a file
void LSystem::parseFile(std::string filename) {
	std::ifstream file(filename);
	if (!file.is_open())
		throw std::runtime_error("failed to open " + filename);

	// Preprocess to remove comments & whitespace
	std::stringstream ss = preprocessStream(file);
	parse(ss);
}

// Apply rules to the latest string to generate the next string
unsigned int LSystem::iterate() {
	if (strings.empty()) return 0;

	// Apply rules to last string
	std::string newString = applyRules(strings.back());
	// Get geometry of new iteration
	auto verts = createGeometry(newString);

	// Check for too-large buffer
	auto& id = iterData.back();
	if ((id.first + id.count + verts.size()) * sizeof(glm::vec3) > MAX_BUF)
		throw std::runtime_error("geometry exceeds maximum buffer size");


	// Store new iteration
	strings.push_back(newString);
	addVerts(verts);

	return getNumIter();
}

// Draw the latest iteration of the L-System
void LSystem::draw(glm::mat4 viewProj) {
	if (!getNumIter()) return;
	drawIter(getNumIter() - 1, viewProj);
}

// Draw a specific iteration of the L-System
void LSystem::drawIter(unsigned int iter, glm::mat4 viewProj) {
	IterData& id = iterData.at(iter);

	glUseProgram(shader);
	glBindVertexArray(vao);

	// Send matrix to shader
	glm::mat4 xform = viewProj * id.bbfix;
	glUniformMatrix3fv(xformLoc, 1, GL_FALSE, glm::value_ptr(xform));
	// Draw L-System
	glDrawArrays(GL_LINES, id.first, id.count);

	glBindVertexArray(0);
	glUseProgram(0);
}

// Apply rules to a given string and return the result
std::string LSystem::applyRules(std::string string) {

	// TODO: ==================================================================
	// Apply rules to the input string
	// Return the resulting string
	std::string ret;
	for(auto ch : string){
		if(rules.count(ch)){
			ret += rules[ch];
		}
		else{
			ret += ch;
		}
	}
	// Replace this line with your implementation
	return ret;
}

// Generate the geometry corresponding to the string at the given iteration
std::vector<glm::vec3> LSystem::createGeometry(std::string string) {
	std::vector<glm::vec3> verts;

	// TODO: ==================================================================
	// Generate geometry from a string
	// Return a vector of vertices, with every two vertices making a 2D line
	// segment.
	glm::vec3 curr = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 prev = glm::vec3(0.0f, 0.0f, 0.0f);
	float ang = 90;
	std::stack<glm::vec3> curr_stack;
	std::stack<glm::vec3> prev_stack;
	std::stack<float> ang_stack;
	float pi = 3.1415926;
	//float ang = angle * (math.pi/180) + (math.pi/2);
	for(auto ch : string){
		if (ch == 'f' || ch == 'F' || ch == 'g' || ch == 'G') {
			curr[0] = cos((ang*pi)/180) + curr[0];
			curr[1] = sin((ang*pi)/180) + curr[1];
			verts.push_back(prev);
			verts.push_back(curr);
			prev = curr;
		}
		else if (ch == 's' || ch == 'S') {
			curr[0] = cos((ang*pi)/180) + curr[0];
			curr[1] = sin((ang*pi)/180) + curr[1];
			prev = curr;
		}
		else if(ch == '+'){
			ang += angle;
		}
		else if(ch == '-'){
			ang -= angle;
		}
		else if(ch == '['){
			prev_stack.push(prev);
			curr_stack.push(curr);
			ang_stack.push(ang);
		}
		else if(ch == ']'){
			prev = prev_stack.top();
			curr = curr_stack.top();
			ang = ang_stack.top();
			prev_stack.pop();
			curr_stack.pop();
			ang_stack.pop();
		}
	}
	return verts;
}

// Add given geometry to the OpenGL vertex buffer and update state accordingly
void LSystem::addVerts(std::vector<glm::vec3>& verts) {
	// Add iteration data
	IterData id;
	if (iterData.empty())
		id.first = 0;
	else {
		auto& lastID = iterData.back();
		id.first = lastID.first + lastID.count;
	}
	id.count = verts.size();

	// Calculate bounding box and create adjustment matrix
	glm::vec3 minBB = glm::vec3(std::numeric_limits<float>::max());
	glm::vec3 maxBB = glm::vec3(std::numeric_limits<float>::lowest());
	for (auto& v : verts) {
		minBB = glm::min(minBB, v);
		maxBB = glm::max(maxBB, v);
	}
	glm::vec3 diag = maxBB - minBB;
	float scale = 1.9f / glm::max(diag.x, diag.y);
	id.bbfix = glm::mat4(1.0f);
	id.bbfix[0][0] = scale;
	id.bbfix[1][1] = scale;
	id.bbfix[2][2] = scale;
	id.bbfix[3] = glm::vec4(-(minBB + maxBB) * scale / 2.0f, 1.0f);
	iterData.push_back(id);

	GLsizei newSize = (id.first + id.count) * sizeof(glm::vec3);
	if (newSize > bufSize) {
		// Create a new vertex buffer to hold vertex data
		GLuint tempBuf;
		glGenBuffers(1, &tempBuf);
		glBindBuffer(GL_ARRAY_BUFFER, tempBuf);
		glBufferData(GL_ARRAY_BUFFER,
			newSize, nullptr, GL_STATIC_DRAW);

		// Copy data from existing buffer
		if (vbo) {
			glBindBuffer(GL_COPY_READ_BUFFER, vbo);
			glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_ARRAY_BUFFER, 0, 0, bufSize);
			glBindBuffer(GL_COPY_READ_BUFFER, 0);
			glDeleteBuffers(1, &vbo);
		}

		vbo = tempBuf;
		bufSize = newSize;

	} else
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

	// Upload new vertex data
	glBufferSubData(GL_ARRAY_BUFFER,
		id.first * sizeof(glm::vec3), id.count * sizeof(glm::vec3), verts.data());


	// Reset vertex data source (format)
	if (!vao) {
		glGenVertexArrays(1, &vao);
	}
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (GLvoid*)0);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Compile and link shader
void LSystem::initShader() {
	std::vector<GLuint> shaders;
	shaders.push_back(compileShader(GL_VERTEX_SHADER, "shaders/v.glsl"));
	shaders.push_back(compileShader(GL_FRAGMENT_SHADER, "shaders/f.glsl"));
	shader = linkProgram(shaders);
	// Cleanup extra state
	for (auto s : shaders)
		glDeleteShader(s);
	shaders.clear();

	// Get uniform locations
	xformLoc = glGetUniformLocation(shader, "xform");
}


// Remove empty lines, comments, and trim leading and trailing whitespace
std::stringstream preprocessStream(std::istream& istr) {
	istr.exceptions(istr.badbit | istr.failbit);
	std::stringstream ss;

	try {
		while (true) {
			std::string line = getNextLine(istr);
			ss << line << std::endl;	// Add newline after each line
		}								// Stream always ends with a newline

	} catch (const std::exception& e) {
		if (!istr.eof()) throw e;
	}

	return ss;
}

// Reads lines from istream, stripping whitespace and comments,
// until it finds a nonempty line
std::string getNextLine(std::istream& istr) {
	const std::string comment = "#";
	std::string line = "";
	while (line == "") {
		std::getline(istr, line);
		// Skip comments and empty lines
		auto found = line.find(comment);
		if (found != std::string::npos)
			line = line.substr(0, found);
		line = trim(line);
	}
	return line;
}

// Trim leading and trailing whitespace from a line
std::string trim(const std::string& line) {
	const std::string whitespace = " \t\r\n";
	auto first = line.find_first_not_of(whitespace);
	if (first == std::string::npos)
		return "";
	auto last = line.find_last_not_of(whitespace);
	auto range = last - first + 1;
	return line.substr(first, range);
}
