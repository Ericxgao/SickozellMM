#define TRIG_MODE 1
#define CV_MODE 0
#define GODOWN 0
#define GORANDOM 1
#define GOUP 2

#include "plugin.hpp"

struct MultiSwitcherML : Module {
	
	int mode = TRIG_MODE;
	float trigValue = 0.f;
	float prevTrigValue = 0.f;

	int currAddr = 0;
	int prevAddr = 0;

	int direction = GODOWN;

	float xFadeKnob = 0.f;
	float xFadeCoeff;

	int fadingIn = -1;
	bool fadingOut[8] = {false, false, false, false, false, false, false, false};
	float xFadeValue[8] = {1, 1, 1, 1, 1, 1, 1, 1};

	int currInput = 0;
	int prevInput = 0;

	int rstIn = 1;
	float rstTrig = 0.f;
	float prevRstTrig = 0.f;

	int maxInputs = 7;

	//int chanL;
	//int chanR;

	//int maxChanL;
	//int maxChanR;

	//float inL[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	//float inR[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	float inL;
	//float inR;

	bool revAdv = false;
	bool cycle = true;
	bool initStart = false;

	unsigned int sampleRate = APP->engine->getSampleRate();

	enum ParamId {
		MODE_SWITCH,
		INS_PARAM,
		DIR_SWITCH,
		XFD_PARAM,
		RST_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		TRG_CV_INPUT,
		RST_INPUT,
		ENUMS(IN_LEFT_INPUT, 8),
		//ENUMS(IN_RIGHT_INPUT, 8),
		INPUTS_LEN
	};
	enum OutputId {
		OUT_LEFT_OUTPUT,
		//OUT_RIGHT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		ENUMS(IN_LIGHT, 8),
		LIGHTS_LEN
	};

	MultiSwitcherML() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		configSwitch(MODE_SWITCH, 0.f, 1.f, 1.f, "Inputs", {"Cv", "Trig"});

		configInput(TRG_CV_INPUT, "Trg/Cv");

		configSwitch(DIR_SWITCH, 0.f, 2.f, 0.f, "Inputs", {"Down", "Random", "Up"});

		configParam(XFD_PARAM, 0.f,1.f, 0.f, "Crossfade", "ms", 10000.f, 1.f);

		configParam(RST_PARAM, 1.f,8.f, 1.f, "Reset Input");
		paramQuantities[RST_PARAM]->snapEnabled = true;

		configInput(RST_INPUT, "Reset");

		configParam(INS_PARAM, 1.f,8.f, 8.f, "Inputs selector");
		paramQuantities[INS_PARAM]->snapEnabled = true;		

		configInput(IN_LEFT_INPUT, "In1");
		//configInput(IN_RIGHT_INPUT, "In1 R");
		configInput(IN_LEFT_INPUT+1, "In2");
		//configInput(IN_RIGHT_INPUT+1, "In2 R");
		configInput(IN_LEFT_INPUT+2, "In3");
		//configInput(IN_RIGHT_INPUT+2, "In3 R");
		configInput(IN_LEFT_INPUT+3, "In4");
		//configInput(IN_RIGHT_INPUT+3, "In4 R");
		configInput(IN_LEFT_INPUT+4, "In5");
		//configInput(IN_RIGHT_INPUT+4, "In5 R");
		configInput(IN_LEFT_INPUT+5, "In6");
		//configInput(IN_RIGHT_INPUT+5, "In6 R");
		configInput(IN_LEFT_INPUT+6, "In7");
		//configInput(IN_RIGHT_INPUT+6, "In7 R");
		configInput(IN_LEFT_INPUT+7, "In8");
		//configInput(IN_RIGHT_INPUT+7, "In8 R");

		configOutput(OUT_LEFT_OUTPUT, "Out");
		//configOutput(OUT_RIGHT_OUTPUT, "Right");

	}

	void onReset(const ResetEvent &e) override {
		//initStart = false;
		
		prevInput = currInput;
		currInput = 0;
		xFadeCoeff = 1 / (sampleRate * (std::pow(10000.f, params[XFD_PARAM].getValue()) / 1000));
		fadingIn = currInput;

		for (int i = 0; i < 8; i++) {
			lights[IN_LIGHT+i].setBrightness(0.f);
			fadingOut[i] = true;
		}	

		Module::onReset(e);
	}

	void onSampleRateChange() override {
		sampleRate = APP->engine->getSampleRate();
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "initStart", json_boolean(initStart));
		json_object_set_new(rootJ, "revAdv", json_boolean(revAdv));
		json_object_set_new(rootJ, "cycle", json_boolean(cycle));
		json_object_set_new(rootJ, "currInput", json_integer(currInput));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* initStartJ = json_object_get(rootJ, "initStart");
		if (initStartJ)
			initStart = json_boolean_value(initStartJ);

		json_t* revAdvJ = json_object_get(rootJ, "revAdv");
		if (revAdvJ)
			revAdv = json_boolean_value(revAdvJ);

		json_t* cycleJ = json_object_get(rootJ, "cycle");
		if (cycleJ)
			cycle = json_boolean_value(cycleJ);

		if (!initStart) {
			json_t* currInputJ = json_object_get(rootJ, "currInput");
			if (currInputJ) {
				currInput = json_integer_value(currInputJ);
				if (currInput >= 0 && currInput < 8) {
					xFadeCoeff = 1 / (sampleRate * (std::pow(10000.f, params[XFD_PARAM].getValue()) / 1000));
					xFadeValue[currInput] = 0;
					fadingIn = currInput;
				} else {
					currInput = params[RST_PARAM].getValue()-1;
					xFadeCoeff = 1 / (sampleRate * (std::pow(10000.f, params[XFD_PARAM].getValue()) / 1000));
					xFadeValue[currInput] = 0;
					fadingIn = currInput;
				}
			} else {
				currInput = params[RST_PARAM].getValue()-1;
				xFadeCoeff = 1 / (sampleRate * (std::pow(10000.f, params[XFD_PARAM].getValue()) / 1000));
				xFadeValue[currInput] = 0;
				fadingIn = currInput;
			}
		} else {
			currInput = params[RST_PARAM].getValue()-1;
			xFadeCoeff = 1 / (sampleRate * (std::pow(10000.f, params[XFD_PARAM].getValue()) / 1000));
			xFadeValue[currInput] = 0;
			fadingIn = currInput;
		}
	}

	void process(const ProcessArgs& args) override {
		
		maxInputs = params[INS_PARAM].getValue();
		
		mode = params[MODE_SWITCH].getValue();

		direction = params[DIR_SWITCH].getValue();

		rstTrig = inputs[RST_INPUT].getVoltage();
		if (rstTrig > 1.f && prevRstTrig <= 1.f) {
			prevInput = currInput;
			lights[IN_LIGHT+prevInput].setBrightness(0.f);

			if (revAdv && mode == TRIG_MODE) {
				switch (direction) {
					case GOUP:
						if (currInput > maxInputs - 2) { 
							if (cycle)
								currInput = 0;
						} else
							currInput++;
					break;

					case GODOWN:
						if (currInput < 1) {
							if (cycle)
								currInput = maxInputs - 1;
						} else
							currInput--;
					break;

					case GORANDOM:
						currInput = random::uniform() * maxInputs;
						if (currInput > maxInputs)
							currInput = maxInputs - 1;
					break;
				}
				xFadeKnob = params[XFD_PARAM].getValue();

				if (xFadeKnob == 0) {
					xFadeValue[currInput] = 1;
				} else {
					xFadeCoeff = 1 / (sampleRate * (std::pow(10000.f, xFadeKnob) / 1000));
					fadingIn = currInput;
					fadingOut[prevInput] = true;
				}

			} else {
				currInput = params[RST_PARAM].getValue() - 1;

				xFadeKnob = params[XFD_PARAM].getValue();

				if (xFadeKnob == 0) {
					xFadeValue[currInput] = 1;
				} else {
					xFadeCoeff = 1 / (sampleRate * (std::pow(10000.f, xFadeKnob) / 1000));
					fadingIn = currInput;
					fadingOut[prevInput] = true;
				}
			}
		}
		prevRstTrig = rstTrig;

		switch (mode) {
			case TRIG_MODE:
				trigValue = inputs[TRG_CV_INPUT].getVoltage();
				if (trigValue > 1.f && prevTrigValue <= 1.f) {
					prevInput = currInput;
					lights[IN_LIGHT+prevInput].setBrightness(0.f);
					switch (direction) {
						case GODOWN:
							if (currInput > maxInputs - 2) { 
								if (cycle)
									currInput = 0;
							} else
								currInput++;
						break;

						case GOUP:
							if (currInput < 1) {
								if (cycle)
									currInput = maxInputs - 1;
							} else
								currInput--;
						break;

						case GORANDOM:
							currInput = random::uniform() * maxInputs;
							if (currInput > maxInputs)
								currInput = maxInputs- 1;
						break;
					}

					xFadeKnob = params[XFD_PARAM].getValue();

					if (xFadeKnob == 0) {
						xFadeValue[currInput] = 1;
					} else {
						xFadeCoeff = 1 / (sampleRate * (std::pow(10000.f, xFadeKnob) / 1000));
						fadingIn = currInput;
						fadingOut[prevInput] = true;
					}
				}
				prevTrigValue = trigValue;

			break;

			case CV_MODE:
				trigValue = inputs[TRG_CV_INPUT].getVoltage();
				if (trigValue > 10.f)
					trigValue = 10.f;
				else if (trigValue < 0.f)
					trigValue = 0.f;

				if (trigValue != prevTrigValue) {
					currAddr = int(trigValue / 10 * maxInputs);
					if (currAddr >= maxInputs)
						currAddr = maxInputs-1;
					if (currAddr != prevAddr) {
						prevInput = currInput;
						lights[IN_LIGHT+prevInput].setBrightness(0.f);
						currInput = currAddr;
						
						prevAddr = currAddr;

						xFadeKnob = params[XFD_PARAM].getValue();

						if (xFadeKnob == 0) {
							xFadeValue[currInput] = 1;
						} else {
							xFadeCoeff = 1 / (sampleRate * (std::pow(10000.f, xFadeKnob) / 1000));
							fadingIn = currInput;
							fadingOut[prevInput] = true;
						}
					}
				}
				prevTrigValue = trigValue;
			break;
		}

		//maxChanL = 0;
		//maxChanR = 0;

		//for (int c = 0; c < 16; c++) {
		//	inL[c] = 0;
		//	inR[c] = 0;
		//}
		inL = 0;
		//inR = 0;

		for (int in = 0; in < 8; in++) {

			//if (inputs[IN_LEFT_INPUT+in].isConnected() || inputs[IN_RIGHT_INPUT+in].isConnected()) {
			if (inputs[IN_LEFT_INPUT+in].isConnected()) {

				//chanL = inputs[IN_LEFT_INPUT+in].getChannels();
				//if (chanL > maxChanL)
				//	maxChanL = chanL;

				if (in == currInput) {

					if (fadingIn == in) {
						xFadeValue[in] += xFadeCoeff;
						if (xFadeValue[in] > 1) {
							xFadeValue[in] = 1;
							fadingIn = -1;
						}
					}

					//for (int c = 0; c < chanL; c++)
						inL += inputs[IN_LEFT_INPUT+in].getVoltage() * xFadeValue[in];

					/*
					if (inputs[IN_RIGHT_INPUT+in].isConnected()) {
						//chanR = inputs[IN_RIGHT_INPUT+in].getChannels();
						//if (chanR > maxChanR)
						//	maxChanR = chanR;

						//for (int c = 0; c < chanR; c++)
							inR += inputs[IN_RIGHT_INPUT+in].getVoltage() * xFadeValue[in];

					} else {
						//chanR = chanL;
						//if (chanR > maxChanR)
						//	maxChanR = chanR;

						//for (int c = 0; c < chanR; c++)
							inR += inputs[IN_LEFT_INPUT+in].getVoltage() * xFadeValue[in];

					}
					*/

				} else {	// if not the current track, check if it's fading out

					if (fadingOut[in]) {

						//for (int c = 0; c < chanL; c++)
							inL += inputs[IN_LEFT_INPUT+in].getVoltage() * xFadeValue[in];
					
						/*
						if (inputs[IN_RIGHT_INPUT+in].isConnected()) {
							//chanR = inputs[IN_RIGHT_INPUT+in].getChannels();
							//if (chanR > maxChanR)
							//	maxChanR = chanR;

							//for (int c = 0; c < chanR; c++)
								inR += inputs[IN_RIGHT_INPUT+in].getVoltage() * xFadeValue[in];

						} else {

							//chanR = chanL;
							//if (chanR > maxChanR)
							//	maxChanR = chanR;

							//for (int c = 0; c < chanR; c++)
								inR += inputs[IN_LEFT_INPUT+in].getVoltage() * xFadeValue[in];

						}
						*/
						xFadeValue[in] -= xFadeCoeff;
						if (xFadeValue[in] < 0) {
							xFadeValue[in] = 0;
							fadingOut[in] = false;
						}

					}

				}

			}

		}

		/*
		for (int c = 0; c < maxChanL; c++) {
			if (inL[c] > 10.f)
				inL[c] = 10.f;
			else if (inL[c] < -10.f)
				inL[c] = -10.f;
			outputs[OUT_LEFT_OUTPUT].setVoltage(inL[c], c);
		}*/
		if (inL > 10.f)
			inL = 10.f;
		else if (inL < -10.f)
			inL = -10.f;
		outputs[OUT_LEFT_OUTPUT].setVoltage(inL);

		/*
		for (int c = 0; c < maxChanR; c++) {
			if (inR[c] > 10.f)
				inR[c] = 10.f;
			else if (inR[c] < -10.f)
				inR[c] = -10.f;
			outputs[OUT_RIGHT_OUTPUT].setVoltage(inR[c], c);
		}
		*/
		/*
		if (inR > 10.f)
			inR = 10.f;
		else if (inR < -10.f)
			inR = -10.f;
		outputs[OUT_RIGHT_OUTPUT].setVoltage(inR);
		*/
		//outputs[OUT_LEFT_OUTPUT].setChannels(maxChanL);
		//outputs[OUT_RIGHT_OUTPUT].setChannels(maxChanR);

		lights[IN_LIGHT+currInput].setBrightness(1.f);

	}		
};

struct MultiSwitcherMLWidget : ModuleWidget {
	MultiSwitcherMLWidget(MultiSwitcherML* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/MultiSwitcherML.svg")));

		/*addChild(createWidget<ScrewBlack>(Vec(0, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		*/
		const float xLeft = 6.8;
		const float yCvSw = 19.5;
		const float yCvSwIn = 35;

		const float xDir = 5.8;
		const float yDir = 59.8;

		const float yXfd = 85.6;
		const float yRstKn = 105.7;
		const float yRstIn = 116;

		//const float xIns = 28.5;
		//const float yIns = 15.6;
		const float xIns = 19.1;
		const float yIns = 17.6;

		//const float xInL = 19.25;
		const float xInL = 19.3;
		//const float xInR = 29.05;
		const float xLight = 15.3;

		//constexpr float ys = 26.5;
		constexpr float ys = 28;
		constexpr float yShift = 10;

		const float yOut = 116;

		addParam(createParamCentered<CKSS>(mm2px(Vec(xLeft, yCvSw)), module, MultiSwitcherML::MODE_SWITCH));
		addInput(createInputCentered<SickoInPort>(mm2px(Vec(xLeft, yCvSwIn)), module, MultiSwitcherML::TRG_CV_INPUT));
		addParam(createParamCentered<CKSSThree>(mm2px(Vec(xDir, yDir)), module, MultiSwitcherML::DIR_SWITCH));

		addParam(createParamCentered<SickoTrimpot>(mm2px(Vec(xLeft, yXfd)), module, MultiSwitcherML::XFD_PARAM));

		addParam(createParamCentered<SickoTrimpot>(mm2px(Vec(xLeft, yRstKn)), module, MultiSwitcherML::RST_PARAM));
		addInput(createInputCentered<SickoInPort>(mm2px(Vec(xLeft, yRstIn)), module, MultiSwitcherML::RST_INPUT));

		addParam(createParamCentered<SickoSmallKnob>(mm2px(Vec(xIns, yIns)), module, MultiSwitcherML::INS_PARAM));

		for (int i = 0; i < 8; i++) {
			addInput(createInputCentered<SickoInPort>(mm2px(Vec(xInL, ys+(i*yShift))), module, MultiSwitcherML::IN_LEFT_INPUT+i));
			//addInput(createInputCentered<SickoInPort>(mm2px(Vec(xInR, ys+(i*yShift))), module, MultiSwitcherML::IN_RIGHT_INPUT+i));
			addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(xLight, ys+(i*yShift)-4)), module, MultiSwitcherML::IN_LIGHT+i));
		}

		addOutput(createOutputCentered<SickoOutPort>(mm2px(Vec(xInL, yOut)), module, MultiSwitcherML::OUT_LEFT_OUTPUT));
		//addOutput(createOutputCentered<SickoOutPort>(mm2px(Vec(xInR, yOut)), module, MultiSwitcherML::OUT_RIGHT_OUTPUT));

	}

	void appendContextMenu(Menu* menu) override {
		MultiSwitcherML* module = dynamic_cast<MultiSwitcherML*>(this->module);

		menu->addChild(new MenuSeparator());
		menu->addChild(createBoolPtrMenuItem("Cycle", "", &module->cycle));
		menu->addChild(createBoolPtrMenuItem("RST input = reverse advance", "", &module->revAdv));
		menu->addChild(new MenuSeparator());
		menu->addChild(createBoolPtrMenuItem("Initialize on Start", "", &module->initStart));

	}
};

Model* modelMultiSwitcherML = createModel<MultiSwitcherML, MultiSwitcherMLWidget>("MultiSwitcherML");