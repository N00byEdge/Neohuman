#include "ModularNN.h"

#include <cmath>
#include <sstream>

template <typename T>
T read(std::istream &is) {
	T temp;
	is >> temp;
	return temp;
}

inline float **makeWeights(const unsigned inSize, const unsigned outSize) {
	float **mat = new float*[inSize];
	for (unsigned i = 0; i < inSize; ++i)
		mat[i] = new float[outSize];

	return mat;
}

template<ModularNN::ActivationFunction actFunc>
ModularNN::StandardLayer<actFunc>::StandardLayer(unsigned inputSize, unsigned outputSize) : inputSize(inputSize), outputSize(outputSize) {
	genWeights();
}

template<ModularNN::ActivationFunction actFunc>
inline ModularNN::StandardLayer<actFunc>::StandardLayer(std::istream &is) :
	inputSize(read<unsigned>(is)),
	outputSize(read<unsigned>(is)),
	weights(makeWeights(inputSize + 1, outputSize)) {

	for (unsigned i = 0; i <= inputSize; ++i)
		for (unsigned j = 0; j < outputSize; ++j)
			is >> weights[i][j];
}

template<ModularNN::ActivationFunction actFunc>
ModularNN::StandardLayer<actFunc>::~StandardLayer() {
	for (unsigned i = 0; i <= inputSize; ++i)
		delete[] weights[i];

	delete[] weights;
}

template<ModularNN::ActivationFunction actFunc>
void ModularNN::applyActivationFunction(fv &v) {
	return applyActivationFunction<actFunc>(v.data(), v.size());
}

template<>
void ModularNN::applyActivationFunction<ModularNN::ActivationFunction::ReLU>(float * const data, const unsigned size) {
	for (unsigned i = 0; i < size; ++i)
		if (data[i] < 0.0f)
			data[i] *= 0.01f;
}

template<>
void ModularNN::applyActivationFunction<ModularNN::ActivationFunction::Sigmoid>(float * const data, const unsigned size) {
	for (unsigned i = 0; i < size; ++i)
		data[i] = 1.0f / (exp(-data[i]) + 1.0f);
}

template<>
void ModularNN::applyActivationFunction<ModularNN::ActivationFunction::Tanh>(float * const data, const unsigned size) {
	for (unsigned i = 0; i < size; ++i)
		data[i] = (2.0f / (1 + exp(-2.0f*data[i]))) - 1.0f;
}

template<>
void ModularNN::applyActivationFunction<ModularNN::ActivationFunction::Softmax>(float * const data, const unsigned size) {
	float sum = 0.0f;
	for (unsigned i = 0; i < size; ++i)
		sum += data[i];
	for (unsigned i = 0; i < size; ++i)
		data[i] /= sum;
}

template <ModularNN::ActivationFunction actf>
fv ModularNN::StandardLayer<actf>::run(fv &in) {
	fv output(outputSize);

	mulWeightsVec(weights, in.data(), output.data(), inputSize, outputSize);
	applyActivationFunction<actf>(output);

	return output;
}

template<>
void ModularNN::StandardLayer<ModularNN::ActivationFunction::ReLU>::write(std::ostream &os) {
	os << "ReLU";
	writeWeights(os);
}

template<>
void ModularNN::StandardLayer<ModularNN::ActivationFunction::Sigmoid>::write(std::ostream &os) {
	os << "Sigmoid";
	writeWeights(os);
}

template<>
void ModularNN::StandardLayer<ModularNN::ActivationFunction::Softmax>::write(std::ostream &os) {
	os << "Softmax";
	writeWeights(os);
}

template<>
void ModularNN::StandardLayer<ModularNN::ActivationFunction::Tanh>::write(std::ostream &os) {
	os << "Tanh";
	writeWeights(os);
}

template<>
void ModularNN::StandardLayer<ModularNN::ActivationFunction::ReLU>::genWeights() {

}

template<ModularNN::ActivationFunction actFunc>
void ModularNN::StandardLayer<actFunc>::writeWeights(std::ostream &os) {
	for (auto &wv : weights)
		for (auto &w : wv)
			os << " " << w;
	os << "\n";
}

void ModularNN::mulMatrixVec(float const * const * const mat, float const * const vec, float * const dest, const unsigned inSize, const unsigned outSize) {
	for (unsigned i = 0; i < inSize; ++i)
		for (unsigned o = 0; o < outSize; ++o)
			dest[o] += mat[i][o] * vec[i];
}

void ModularNN::mulWeightsVec(float const * const *weights, float const * const vec, float * const dest, const unsigned inSize, const unsigned outSize) {
	for (unsigned i = 0; i < inSize; ++i)
		for (unsigned o = 0; o < outSize; ++o)
			dest[o] += weights[i][o] * vec[i];

	for (unsigned o = 0; o < outSize; ++o) // bias
		dest[o] += weights[inSize][o];
}

void ModularNN::addVectors(float *const target, float const *const source, const unsigned count) {
	for (unsigned i = 0; i < count; ++i)
		target[i] += source[i];
}

ModularNN::ModularNN(std::istream &is) {
	std::string line;
	while (getline(is, line)) {
		auto layer = std::move(readLayer(std::stringstream(line)));
		if (layer)
			layers.push_back(std::move(layer));
	}
}

fv ModularNN::run(fv & input) {
	for (auto &l : layers)
		input = l->run(input);
	return input;
}

std::unique_ptr<ModularNN::Layer> ModularNN::readLayer(std::istream &is) {
	std::string layerType = "";
	is >> layerType;
	if (layerType == "ReLU")
		return std::make_unique<ModularNN::StandardLayer<ModularNN::ActivationFunction::ReLU>>(is);
	if (layerType == "Sigmoid")
		return std::make_unique<ModularNN::StandardLayer<ModularNN::ActivationFunction::Sigmoid>>(is);
	if (layerType == "Softmax")
		return std::make_unique<ModularNN::StandardLayer<ModularNN::ActivationFunction::Softmax>>(is);
	if (layerType == "Tanh")
		return std::make_unique<ModularNN::StandardLayer<ModularNN::ActivationFunction::Tanh>>(is);

	return nullptr;
}

ModularNN::LSTM::LSTM(std::istream &is) : inputSize(read<unsigned>(is)), outputSize(read<unsigned>(is)), mSize(read<unsigned>(is)),
Wmx(makeWeights(inputSize, mSize)), Wmh(makeWeights(outputSize, mSize)), Whx(makeWeights(inputSize, outputSize)), Whm(makeWeights(mSize, outputSize)), Wix(makeWeights(inputSize, outputSize)), Wim(makeWeights(mSize, outputSize)), Wox(makeWeights(inputSize, outputSize)), Wom(makeWeights(mSize, outputSize)), Wfx(makeWeights(inputSize, outputSize)), Wfm(makeWeights(mSize, outputSize)),
	bm(new float[mSize]), bhr(new float[outputSize]), bi(new float[outputSize]), bo(new float[outputSize]), bf(new float[outputSize]), bc(new float[outputSize]), bh(new float[outputSize]), hState(new float[outputSize]), cState(new float[outputSize]) {
	for (unsigned i = 0; i < inputSize; ++i)
		for (unsigned m = 0; m < mSize; ++m)
			is >> Wmx[i][m];

	for (unsigned o = 0; o < outputSize; ++o)
		for (unsigned m = 0; m < mSize; ++m)
			is >> Wmh[o][m];

	for (unsigned i = 0; i < inputSize; ++i)
		for (unsigned o = 0; o < outputSize; ++o)
			is >> Whx[i][o];

	for (unsigned m = 0; m < mSize; ++m)
		for (unsigned o = 0; o < outputSize; ++o)
			is >> Whm[m][o];

	for (unsigned i = 0; i < inputSize; ++i)
		for (unsigned o = 0; o < outputSize; ++o)
			is >> Wix[i][o];

	for (unsigned m = 0; m < mSize; ++m)
		for (unsigned o = 0; o < outputSize; ++o)
			is >> Wim[m][o];

	for (unsigned i = 0; i < inputSize; ++i)
		for (unsigned o = 0; o < outputSize; ++o)
			is >> Wox[i][o];

	for (unsigned m = 0; m < mSize; ++m)
		for (unsigned o = 0; o < outputSize; ++o)
			is >> Wom[m][o];

	for (unsigned i = 0; i < inputSize; ++i)
		for (unsigned o = 0; o < outputSize; ++o)
			is >> Wfx[i][o];

	for (unsigned m = 0; m < mSize; ++m)
		for (unsigned o = 0; o < outputSize; ++o)
			is >> Wfm[m][o];

	for (unsigned m = 0; m < mSize; ++m)
		is >> bm[m];

	for (unsigned o = 0; o < outputSize; ++o)
		is >> bhr[o];

	for (unsigned o = 0; o < outputSize; ++o)
		is >> bi[o];

	for (unsigned o = 0; o < outputSize; ++o)
		is >> bo[o];

	for (unsigned o = 0; o < outputSize; ++o)
		is >> bf[o];

	for (unsigned o = 0; o < outputSize; ++o)
		is >> bc[o];

	for (unsigned o = 0; o < outputSize; ++o)
		is >> bh[o];

	for (unsigned o = 0; o < outputSize; ++o)
		hState[o] = 0.0f, cState[o] = 0.0f;
}

ModularNN::LSTM::~LSTM() {
	// Delete weight vectors
	for (unsigned i = 0; i < inputSize; ++i)
		delete[] Wmx[i], delete[] Whx[i], delete[] Wix[i], delete[] Wox[i], delete[] Wfx[i];

	for (unsigned o = 0; o < outputSize; ++o)
		delete[] Wmh[o];

	for (unsigned m = 0; m < mSize; ++m)
		delete[] Whm[m], delete[] Wim[m], delete[] Wom[m], delete[] Wfm[m];

	// Delete weight vector vectors
	for (auto &wm : { Wmx, Wmh, Whx, Whm, Wix, Wim, Wox, Wom, Wfx, Wfm })
		delete[] wm;
	
	// Delete biases
	for (auto &bv : { bm, bhr, bi, bo, bf, bc, bh })
		delete[] bv;
}

fv ModularNN::LSTM::run(fv &input) {
	auto m = new float[mSize]();
	mulMatrixVec(Wmx, input.data(), m, inputSize, mSize);
	mulMatrixVec(Wmh, hState, m, outputSize, mSize);
	addVectors(m, bm, mSize);

	auto hr = new float[outputSize]();
	mulMatrixVec(Whx, input.data(), hr, inputSize, outputSize);
	mulMatrixVec(Whm, m, hr, mSize, outputSize);
	addVectors(m, bm, outputSize);

	auto i = new float[outputSize]();
	mulMatrixVec(Wix, input.data(), i, inputSize, outputSize);
	mulMatrixVec(Wim, m, i, mSize, outputSize);
	addVectors(i, bi, outputSize);
	applyActivationFunction<ModularNN::ActivationFunction::Sigmoid>(i, outputSize);

	auto o = new float[outputSize]();
	mulMatrixVec(Wox, input.data(), o, inputSize, outputSize);
	mulMatrixVec(Wom, m, o, mSize, outputSize);
	addVectors(o, bo, outputSize);
	applyActivationFunction<ModularNN::ActivationFunction::Sigmoid>(o, outputSize);

	auto f = new float[outputSize]();
	mulMatrixVec(Wfx, input.data(), f, inputSize, outputSize);
	mulMatrixVec(Wfm, m, f, mSize, outputSize);
	addVectors(f, bf, outputSize);
	applyActivationFunction<ModularNN::ActivationFunction::Sigmoid>(f, outputSize);

	for (unsigned on = 0; on < outputSize; ++on)
		cState[on] = f[on] * cState[on] + i[on] * hr[on] + bc[on], hState[on] = cState[on] * o[on] + bh[on];
	applyActivationFunction<ModularNN::ActivationFunction::Tanh>(hState, outputSize);

	return fv(hState, hState + outputSize);
}

void ModularNN::LSTM::write(std::ostream &) {
}
