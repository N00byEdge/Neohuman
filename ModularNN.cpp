#include "ModularNN.h"

#include <cmath>
#include <sstream>
#include <random>
#include <chrono>

#ifdef WIN32
std::mt19937_64 nnMt(std::chrono::system_clock::now().time_since_epoch().count());
#else
std::mt19937_64 nnMt(std::chrono::steady_clock::now().time_since_epoch().count());
#endif

template <typename T>
T read(std::istream &is) {
	T temp;
	is >> temp;
	return temp;
}

template<ModularNN::ActivationFunction actFunc>
ModularNN::StandardLayer<actFunc>::StandardLayer(unsigned inputSize, unsigned outputSize) :
	inputSize(inputSize),
	outputSize(outputSize),
	weights(new float[(inputSize + 1) * outputSize]) {
	genWeights();
}

template<ModularNN::ActivationFunction actFunc>
inline ModularNN::StandardLayer<actFunc>::StandardLayer(std::istream &is) :
	inputSize(read<unsigned>(is)),
	outputSize(read<unsigned>(is)),
	weights(new float[(inputSize + 1) * outputSize]) {

	for (unsigned i = 0; i <= inputSize; ++i)
		for (unsigned j = 0; j < outputSize; ++j)
			is >> weights[i * outputSize + j];
}

template<ModularNN::ActivationFunction actFunc>
ModularNN::StandardLayer<actFunc>::~StandardLayer() {
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

template<ModularNN::ActivationFunction actFunc>
void ModularNN::StandardLayer<actFunc>::genWeights() {
	ModularNN::initWeightMatrix<actFunc>(weights, inputSize + 1, outputSize);
}

template<ModularNN::ActivationFunction actFunc>
void ModularNN::StandardLayer<actFunc>::writeWeights(std::ostream &os) {
	os << " " << inputSize << " " << outputSize;
	for (auto i = 0u; i < inputSize + 1; ++i)
		for (auto o = 0u; o < outputSize; ++o)
			os << " " << weights[i * outputSize + o];
	os << "\n";
}

void ModularNN::mulMatrixVec(float const * const mat, float const * const vec, float * const dest, const unsigned inSize, const unsigned outSize) {
	for (unsigned i = 0; i < inSize; ++i)
		for (unsigned o = 0; o < outSize; ++o)
			dest[o] += mat[i * outSize + o] * vec[i];
}

void ModularNN::mulWeightsVec(float const *weights, float const * const vec, float * const dest, const unsigned inSize, const unsigned outSize) {
	for (unsigned i = 0; i < inSize; ++i)
		for (unsigned o = 0; o < outSize; ++o)
			dest[o] += weights[i * outSize + o] * vec[i];

	for (unsigned o = 0; o < outSize; ++o) // bias
		dest[o] += weights[inSize * outSize + o];
}

void ModularNN::addVectors(float *const target, float const *const source, const unsigned count) {
	for (unsigned i = 0; i < count; ++i)
		target[i] += source[i];
}

template<ModularNN::ActivationFunction actFunc>
void ModularNN::initWeightMatrix(float *const weights, unsigned inputSize, unsigned outputSize) {
	for (auto i = 0u; i < inputSize; ++i)
		initWeightVector<actFunc>(weights + outputSize * i, inputSize, outputSize);
}

template<>
void ModularNN::initWeightVector<ModularNN::ActivationFunction::ReLU>(float *const weights, unsigned inputSize, unsigned outputSize) {
	float r = sqrt(2.0f / (inputSize + outputSize));
	std::uniform_real_distribution<float> dist(-r, r);
	for (auto o = 0u; o < outputSize; ++o)
		weights[o] = dist(nnMt);
}

template<>
void ModularNN::initWeightVector<ModularNN::ActivationFunction::Sigmoid>(float *const weights, unsigned inputSize, unsigned outputSize) {
	float r = sqrt(6.0f / (inputSize + outputSize)) / 4.0f;
	std::uniform_real_distribution<float> dist(-r, r);
	for (auto o = 0u; o < outputSize; ++o)
		weights[o] = dist(nnMt);
}

template<>
void ModularNN::initWeightVector<ModularNN::ActivationFunction::Softmax>(float *const weights, unsigned inputSize, unsigned outputSize) {
	return initWeightVector<ModularNN::ActivationFunction::Sigmoid>(weights, inputSize, outputSize);
}

template<>
void ModularNN::initWeightVector<ModularNN::ActivationFunction::Tanh>(float *const weights, unsigned inputSize, unsigned outputSize) {
	float r = sqrt(6.0f / (inputSize + outputSize));
	std::uniform_real_distribution<float> dist(-r, r);
	for (auto o = 0u; o < outputSize; ++o)
		weights[o] = dist(nnMt);
}

ModularNN::ModularNN(std::vector<std::unique_ptr<Layer>> &layers) {
	for (auto &l : layers)
		this->layers.push_back(std::move(l));
}

ModularNN::ModularNN(std::istream &is) {
	std::string line;
	while (getline(is, line)) {
		auto layer = std::move(readLayer(std::stringstream(line)));
		if (layer)
			layers.push_back(std::move(layer));
	}
}

fv ModularNN::run(fv input) {
	for (auto &l : layers)
		input = l->run(input);
	return input;
}

void ModularNN::write(std::ostream &os) {
	for (auto &l : layers)
		l->write(os);
	os << std::flush;
}

ModularNN ModularNN::genDivNN() {
	std::stringstream in, out;
	write(in);

	std::normal_distribution<double> dist(0.0f, I);
	std::string line;
	while (getline(in, line)) {
		std::string layerType = "";
		std::stringstream sline(line);
		sline >> layerType;
		out << layerType;
		if (layerType == "LSTM") {
			int inSize, outSize, mSize;
			sline >> inSize >> outSize >> mSize;
			out << " " << inSize << " " << outSize << " " << mSize;
		}
		else {
			int inSize, outSize;
			sline >> inSize >> outSize;
			out << " " << inSize << " " << outSize;
		}
		float temp;
		while (sline >> temp)
			out << " " << (float)dist(nnMt) + temp;
		out << std::endl;
	}

	return ModularNN(out);
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
	if (layerType == "LSTM")
		return std::make_unique<ModularNN::LSTM>(is);

	return nullptr;
}

ModularNN::LSTM::LSTM(unsigned inputSize, unsigned outputSize, unsigned mSize) : inputSize(inputSize), outputSize(outputSize), mSize(mSize),
	Wmx(new float[inputSize * mSize]), Wmh(new float[outputSize * mSize]), Whx(new float[inputSize * outputSize]), Whm(new float[mSize * outputSize]), Wix(new float[inputSize * outputSize]), Wim(new float[mSize * outputSize]), Wox(new float[inputSize * outputSize]), Wom(new float[mSize * outputSize]), Wfx(new float[inputSize * outputSize]), Wfm(new float[mSize * outputSize]),
	bm(new float[mSize]), bhr(new float[outputSize]), bi(new float[outputSize]), bo(new float[outputSize]), bf(new float[outputSize]), bc(new float[outputSize]), bh(new float[outputSize]), hState(new float[outputSize]), cState(new float[outputSize]) {

	initWeightMatrix<ActivationFunction::ReLU>(Wmx, inputSize, mSize);
	initWeightMatrix<ActivationFunction::ReLU>(Wmh, outputSize, mSize);
	initWeightVector<ActivationFunction::ReLU>(bm, (inputSize + outputSize)/2, mSize);

	initWeightMatrix<ActivationFunction::ReLU>(Whx, inputSize, outputSize);
	initWeightMatrix<ActivationFunction::ReLU>(Whm, mSize, outputSize);
	initWeightVector<ActivationFunction::ReLU>(bhr, (inputSize + mSize)/2, outputSize);

	initWeightMatrix<ActivationFunction::Sigmoid>(Wix, inputSize, outputSize);
	initWeightMatrix<ActivationFunction::Sigmoid>(Wim, mSize, outputSize);
	initWeightVector<ActivationFunction::Sigmoid>(bi, (inputSize + mSize) / 2, outputSize);

	initWeightMatrix<ActivationFunction::Sigmoid>(Wox, inputSize, outputSize);
	initWeightMatrix<ActivationFunction::Sigmoid>(Wom, mSize, outputSize);
	initWeightVector<ActivationFunction::Sigmoid>(bo, (inputSize + mSize) / 2, outputSize);

	initWeightMatrix<ActivationFunction::Sigmoid>(Wfx, inputSize, outputSize);
	initWeightMatrix<ActivationFunction::Sigmoid>(Wfm, mSize, outputSize);
	initWeightVector<ActivationFunction::Sigmoid>(bf, (inputSize + mSize) / 2, outputSize);

	initWeightVector<ActivationFunction::ReLU>(bc, outputSize, outputSize);
	initWeightVector<ActivationFunction::Tanh>(bh, outputSize, outputSize);

	for (unsigned o = 0; o < outputSize; ++o)
		hState[o] = 0.0f, cState[o] = 0.0f;
}

ModularNN::LSTM::LSTM(std::istream &is) : inputSize(read<unsigned>(is)), outputSize(read<unsigned>(is)), mSize(read<unsigned>(is)),
Wmx(new float[inputSize * mSize]), Wmh(new float[outputSize * mSize]), Whx(new float[inputSize * outputSize]), Whm(new float[mSize * outputSize]), Wix(new float[inputSize * outputSize]), Wim(new float[mSize * outputSize]), Wox(new float[inputSize * outputSize]), Wom(new float[mSize * outputSize]), Wfx(new float[inputSize * outputSize]), Wfm(new float[mSize * outputSize]),
	bm(new float[mSize]), bhr(new float[outputSize]), bi(new float[outputSize]), bo(new float[outputSize]), bf(new float[outputSize]), bc(new float[outputSize]), bh(new float[outputSize]), hState(new float[outputSize]), cState(new float[outputSize]) {
	for (unsigned i = 0; i < inputSize; ++i)
		for (unsigned m = 0; m < mSize; ++m)
			is >> Wmx[i * mSize + m];

	for (unsigned o = 0; o < outputSize; ++o)
		for (unsigned m = 0; m < mSize; ++m)
			is >> Wmh[o * mSize + m];

	for (unsigned i = 0; i < inputSize; ++i)
		for (unsigned o = 0; o < outputSize; ++o)
			is >> Whx[i * outputSize + o];

	for (unsigned m = 0; m < mSize; ++m)
		for (unsigned o = 0; o < outputSize; ++o)
			is >> Whm[m * outputSize + o];

	for (unsigned i = 0; i < inputSize; ++i)
		for (unsigned o = 0; o < outputSize; ++o)
			is >> Wix[i * outputSize + o];

	for (unsigned m = 0; m < mSize; ++m)
		for (unsigned o = 0; o < outputSize; ++o)
			is >> Wim[m + outputSize + o];

	for (unsigned i = 0; i < inputSize; ++i)
		for (unsigned o = 0; o < outputSize; ++o)
			is >> Wox[i * outputSize + o];

	for (unsigned m = 0; m < mSize; ++m)
		for (unsigned o = 0; o < outputSize; ++o)
			is >> Wom[m * outputSize + o];

	for (unsigned i = 0; i < inputSize; ++i)
		for (unsigned o = 0; o < outputSize; ++o)
			is >> Wfx[i * outputSize + o];

	for (unsigned m = 0; m < mSize; ++m)
		for (unsigned o = 0; o < outputSize; ++o)
			is >> Wfm[m * outputSize + o];

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
	// Delete weight matrices
	for (auto wm : { Wmx, Whx, Wix, Wox, Wfx, Wmh, Whm, Wim, Wom, Wfm })
		delete[] wm;
	
	// Delete biases
	for (auto &bv : { bm, bhr, bi, bo, bf, bc, bh, cState, hState })
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

	delete[] m, delete[] hr, delete[] i, delete[] o, delete[] f;

	return fv(hState, hState + outputSize);
}

void ModularNN::LSTM::write(std::ostream &os) {
	os << "LSTM " << inputSize << " " << outputSize << " " << mSize;
	for (unsigned i = 0; i < inputSize; ++i)
		for (unsigned m = 0; m < mSize; ++m)
			os << " " << Wmx[i * mSize + m];

	for (unsigned o = 0; o < outputSize; ++o)
		for (unsigned m = 0; m < mSize; ++m)
			os << " " << Wmh[o * mSize + m];

	for (unsigned i = 0; i < inputSize; ++i)
		for (unsigned o = 0; o < outputSize; ++o)
			os << " " << Whx[i * outputSize + o];

	for (unsigned m = 0; m < mSize; ++m)
		for (unsigned o = 0; o < outputSize; ++o)
			os << " " << Whm[m * outputSize + o];

	for (unsigned i = 0; i < inputSize; ++i)
		for (unsigned o = 0; o < outputSize; ++o)
			os << " " << Wix[i * outputSize + o];

	for (unsigned m = 0; m < mSize; ++m)
		for (unsigned o = 0; o < outputSize; ++o)
			os << " " << Wim[m * outputSize + o];

	for (unsigned i = 0; i < inputSize; ++i)
		for (unsigned o = 0; o < outputSize; ++o)
			os << " " << Wox[i * outputSize + o];

	for (unsigned m = 0; m < mSize; ++m)
		for (unsigned o = 0; o < outputSize; ++o)
			os << " " << Wom[m * outputSize + o];

	for (unsigned i = 0; i < inputSize; ++i)
		for (unsigned o = 0; o < outputSize; ++o)
			os << " " << Wfx[i * outputSize + o];

	for (unsigned m = 0; m < mSize; ++m)
		for (unsigned o = 0; o < outputSize; ++o)
			os << " " << Wfm[m * outputSize + o];

	for (unsigned m = 0; m < mSize; ++m)
		os << " " << bm[m];

	for (unsigned o = 0; o < outputSize; ++o)
		os << " " << bhr[o];

	for (unsigned o = 0; o < outputSize; ++o)
		os << " " << bi[o];

	for (unsigned o = 0; o < outputSize; ++o)
		os << " " << bo[o];

	for (unsigned o = 0; o < outputSize; ++o)
		os << " " << bf[o];

	for (unsigned o = 0; o < outputSize; ++o)
		os << " " << bc[o];

	for (unsigned o = 0; o < outputSize; ++o)
		os << " " << bh[o];

	for (unsigned o = 0; o < outputSize; ++o)
		hState[o] = 0.0f, cState[o] = 0.0f;

	os << "\n";
}

namespace {
	template struct ModularNN::StandardLayer<ModularNN::ActivationFunction::ReLU>;
	template struct ModularNN::StandardLayer<ModularNN::ActivationFunction::Sigmoid>;
	template struct ModularNN::StandardLayer<ModularNN::ActivationFunction::Tanh>;
	template struct ModularNN::StandardLayer<ModularNN::ActivationFunction::Softmax>;
}
