//////////////////////////////////////////////////////////////////////////
/// Description.cc: shader instance description generator implementation.
/// Copyright (C) $(GODDAMN_DEV) 2011 - Present. All Rights Reserved.
/// 
/// History:
///		* 17.05.2014 - Created by James Jhuighuy
//////////////////////////////////////////////////////////////////////////

#include <GoddamnEngine/Engine/Renderer/Shader/CrossCompiler/Validator/Validator.hh>
#include <GoddamnEngine/Engine/Renderer/Shader/CrossCompiler/Parser/Parser.hh>
#include <GoddamnEngine/Engine/Renderer/Shader/CrossCompiler/CrossCompiler.hh>
#include <GoddamnEngine/Core/Containers/Pointer/UniquePtr.hh>

/// Define this to make validator throw warnings on stuff unrecommended to
/// used in GoddamnEngine coding standarts.
#define GD_SHADERCC_VALIDATOR_VALIDATE_STYLE 1

/// Define this to make validator try automatically fix some warnings in parsed data. 
/// @note This is not same because it modifies ParsedData.
// #define GD_SHADERCC_VALIDATOR_AUTOFIX_WARNINGS 1

GD_NAMESPACE_BEGIN

	typedef Str HLSLValidatorErrorDesc;
	typedef Str HLSLValidatorWarningDec;

	bool HLSLValidator::ValidateResourceParameters(HLSLScope const* const ParsedData, HRIShaderInstanceDesc* const ShaderInstanceDesc)
	{
		size_t TextureObjectNewRegister = 0;
		RefPtr<HRIShaderParamLocationDesc> ShaderParamLocationResources(new HRIShaderParamLocationDesc(ShaderInstanceDesc, GD_HRI_SHADER_PARAM_LOCATION_DESC_TYPE_RESOURCES));
		for (auto const Definition : ParsedData->InnerDefinitions)
		{
			HLSLVariable const* const TextureObject = HLSLDynamicCast<HLSLVariable const*>(Definition);
			if (TextureObject == nullptr)
				continue;

			HRIShaderParamDescType TextureObjectType = GD_HRI_SHADER_PARAM_DESC_TYPE_UNKNOWN;
			/**/ if (TextureObject->Type->Class == GD_HLSL_TYPE_CLASS_TEXTURE2D)
				TextureObjectType = GD_HRI_SHADER_PARAM_DESC_TYPE_TEXTURE2D;
			else if (TextureObject->Type->Class == GD_HLSL_TYPE_CLASS_TEXTURECUBE)
				TextureObjectType = GD_HRI_SHADER_PARAM_DESC_TYPE_TEXTURECUBE;
			else
			{
                if (TextureObject->Type->Class != GD_HLSL_TYPE_CLASS_SAMPLER)
                {
                    HLSLVariable const* const StaticVariable = TextureObject;
                    if (StaticVariable->ExprColon != nullptr)
                    {	// Some static variable contains after-colon expression.
                        HLSLValidatorErrorDesc static const HasExprColonError("static variable '%s' cannot have after-colon-experssion.");
                        self->RaiseError(HasExprColonError, &TextureObject->Name[0]);
                        self->RaiseExceptionWithCode(GD_HRI_SHADERCC_EXCEPTION_SYNTAX);

                        return false;
                    }
                }

				continue;
			}

			// Lets check if this texture object also contains sampler.
			HLSLVariable const* const TextureSampler = ParsedData->FindVariable(TextureObject->Name + "Sampler");
			if (TextureSampler == nullptr)
			{	// No sampler was found.
				HLSLValidatorErrorDesc static const NoSamplerWasFoundForTextureError("appropriate sampler for texture object named '%s' does not exists.");
				self->RaiseError(NoSamplerWasFoundForTextureError, &TextureObject->Name[0]);
				self->RaiseExceptionWithCode(GD_HRI_SHADERCC_EXCEPTION_SYNTAX);

				return false;
			}

			HLSLRegister const* const TextureObjectRegister = HLSLDynamicCast<HLSLRegister const*>(TextureObject->ExprColon);
			if (TextureObjectRegister != nullptr)
			if (TextureObjectRegister->Register != GD_HLSL_REGISTER_T)
			{	// Texture is located outside 'T' registers group
				HLSLValidatorErrorDesc static const InvalidTextureRegisterError("texture '%s' is located outside of 'T' registers group.");
				self->RaiseError(InvalidTextureRegisterError, &TextureObject->Name[0]);
				self->RaiseExceptionWithCode(GD_HRI_SHADERCC_EXCEPTION_SYNTAX);

				return false;
			}

			HLSLRegister const* const TextureSamplerRegister = HLSLDynamicCast<HLSLRegister const*>(TextureSampler->ExprColon);
			if (TextureSamplerRegister != nullptr)
			if (TextureSamplerRegister->Register != GD_HLSL_REGISTER_S)
			{	// Sampler is located outside 'S' registers group
				HLSLValidatorErrorDesc static const InvalidTextureRegisterError("sampler '%s' is located outside of 'S' registers group.");
				self->RaiseError(InvalidTextureRegisterError, &TextureSampler->Name[0]);
				self->RaiseExceptionWithCode(GD_HRI_SHADERCC_EXCEPTION_SYNTAX);

				return false;
			}

			size_t TextureObjectUsedRegister = SIZE_MAX;
			if ((TextureObjectRegister != nullptr) && (TextureSamplerRegister != nullptr))
			{	// Both texture object and sampler contain register information.
				if (TextureObjectRegister->RegisterID != TextureSamplerRegister->RegisterID)
				{	// Texture and it`s sampler should be located in same registers
					HLSLValidatorErrorDesc static const DifferentTextureSamplerRegistersError("texture '%s' and appropriate sampler should be located in same registers");
					self->RaiseError(DifferentTextureSamplerRegistersError, &TextureObject->Name[0]);
					self->RaiseExceptionWithCode(GD_HRI_SHADERCC_EXCEPTION_SYNTAX);

					return false;
				}

				TextureObjectUsedRegister = TextureObjectRegister->RegisterID;
				TextureObjectNewRegister = TextureObjectUsedRegister + 1;
			}
			else if ((TextureObjectRegister == nullptr) && (TextureSamplerRegister != nullptr))
			{	// Only texture sampler contains register information.
				HLSLValidatorWarningDec static const NoSamplerRegisterWarning("texture sampler '%s' contains explicit register information, but object does not. Consider adding it.");
				self->RaiseWarning(NoSamplerRegisterWarning, &TextureSampler->Name[0]);
				TextureObjectUsedRegister = TextureSamplerRegister->RegisterID;
				TextureObjectNewRegister = TextureObjectUsedRegister + 1;
#if (defined(GD_SHADERCC_VALIDATOR_AUTOFIX_WARNINGS))
#	 define  GD_SHADERCC_VALIDATOR_AUTOFIX_NULL_TEXTURE_OBJECT_REGISTER() \
		HLSLVariable* const TextureObjectMut = const_cast<HLSLVariable*>(TextureObject); \
		HLSLRegister* const TextureObjectRegisterMut = new HLSLRegister(); \
		TextureObjectRegisterMut->Register = GD_HLSL_REGISTER_T; \
		TextureObjectRegisterMut->RegisterID = TextureObjectUsedRegister; \
		delete TextureObjectMut->ExprColon; \
		TextureObjectMut->ExprColon = TextureObjectRegisterMut; 
				GD_SHADERCC_VALIDATOR_AUTOFIX_NULL_TEXTURE_OBJECT_REGISTER();
#endif	// if (defined(GD_SHADERCC_VALIDATOR_AUTOFIX_WARNINGS))
			}
			else if ((TextureObjectRegister != nullptr) && (TextureSamplerRegister == nullptr))
			{	// Only texture object contains register information.
				HLSLValidatorWarningDec static const NoSamplerRegisterWarning("texture object '%s' contains explicit register information, but sampler does not. Consider adding it.");
				self->RaiseWarning(NoSamplerRegisterWarning, &TextureObject->Name[0]);
				TextureObjectUsedRegister = TextureObjectRegister->RegisterID;
				TextureObjectNewRegister = TextureObjectUsedRegister + 1;
#if (defined(GD_SHADERCC_VALIDATOR_AUTOFIX_WARNINGS))
#	 define  GD_SHADERCC_VALIDATOR_AUTOFIX_NULL_TEXTURE_SAMPLER_REGISTER() \
		HLSLVariable* const TextureSamplerMut = const_cast<HLSLVariable*>(TextureSampler); \
		HLSLRegister* const TextureSamplerRegisterMut = new HLSLRegister(); \
		TextureSamplerRegisterMut->Register = GD_HLSL_REGISTER_S; \
		TextureSamplerRegisterMut->RegisterID = TextureObjectUsedRegister; \
		delete TextureSamplerMut->ExprColon; \
		TextureSamplerMut->ExprColon = TextureSamplerRegisterMut;
				GD_SHADERCC_VALIDATOR_AUTOFIX_NULL_TEXTURE_SAMPLER_REGISTER();
#endif	// if (defined(GD_SHADERCC_VALIDATOR_AUTOFIX_WARNINGS))
			}
			else
			{
				HLSLValidatorWarningDec static const NoExplicitRegisterInformationWarning("texture object '%s' and appropriate sampler does not contain explicit register information. Consider adding it.");
				self->RaiseWarning(NoExplicitRegisterInformationWarning, &TextureObject->Name[0]);
				TextureObjectUsedRegister = TextureObjectNewRegister;
				TextureObjectNewRegister += 1;

#if (defined(GD_SHADERCC_VALIDATOR_AUTOFIX_WARNINGS))
				GD_SHADERCC_VALIDATOR_AUTOFIX_NULL_TEXTURE_OBJECT_REGISTER();
				GD_SHADERCC_VALIDATOR_AUTOFIX_NULL_TEXTURE_SAMPLER_REGISTER();
#	undef GD_SHADERCC_VALIDATOR_AUTOFIX_NULL_TEXTURE_OBJECT_REGISTER
#	undef GD_SHADERCC_VALIDATOR_AUTOFIX_NULL_TEXTURE_SAMPLER_REGISTER
#endif	// if (defined(GD_SHADERCC_VALIDATOR_AUTOFIX_WARNINGS))
			}

			// Now we are sure that texture paramter is valid and ready to create descriptor.
			RefPtr<HRIShaderParamDesc>(new HRIShaderParamDesc(ShaderParamLocationResources.GetPointer(), TextureObject->Name, TextureObjectType, GD_FORMAT_UNKNOWN, TextureObjectUsedRegister));
		}

		return true;
	}

	bool HLSLValidator::ValidateConstantBuffersParameters(HLSLScope const* const ParsedData, HRIShaderInstanceDesc* const ShaderInstanceDesc)
	{
		size_t ConstantBufferNewRegister = 0;
		for (auto const Definition : ParsedData->InnerDefinitions)
		{
			HLSLCBuffer const* const ConstantBuffer = HLSLDynamicCast<HLSLCBuffer const*>(Definition);
			if (ConstantBuffer == nullptr)
				continue;

			HLSLRegister const* const ConstantBufferRegister = ConstantBuffer->Register;
			size_t ConstantBufferUsedRegister = SIZE_MAX;
			if (ConstantBufferRegister != nullptr)
			{
				if (ConstantBufferRegister->Register != GD_HLSL_REGISTER_B)
				{	// Constant buffer is located outside 'B' registers group
					HLSLValidatorErrorDesc static const InvalidTextureRegisterError("constant buffer '%s' is located outside of 'B' registers group.");
					self->RaiseError(InvalidTextureRegisterError, &ConstantBuffer->Name[0]);
					self->RaiseExceptionWithCode(GD_HRI_SHADERCC_EXCEPTION_SYNTAX);

					return false;
				}

				ConstantBufferUsedRegister = ConstantBufferRegister->RegisterID;
				ConstantBufferNewRegister = ConstantBufferUsedRegister + 1;
			}
			else
			{
				HLSLValidatorWarningDec static const NoExplicitRegisterInformationWarning("constant buffer '%s' does not contain explicit register information. Consider adding it.");
				self->RaiseWarning(NoExplicitRegisterInformationWarning, &ConstantBuffer->Name[0]);
				ConstantBufferUsedRegister = ConstantBufferNewRegister;
				ConstantBufferNewRegister += 1;

#if (defined(GD_SHADERCC_VALIDATOR_AUTOFIX_WARNINGS))
				HLSLCBuffer* const ConstantBufferMut = const_cast<HLSLCBuffer*>(ConstantBuffer);
				HLSLRegister* const ConstantBufferRegisterMut = new HLSLRegister();
				ConstantBufferRegisterMut->Register = GD_HLSL_REGISTER_B;
				ConstantBufferRegisterMut->RegisterID = ConstantBufferUsedRegister;
				ConstantBufferMut->Register = ConstantBufferRegisterMut;
#endif	// if (defined(GD_SHADERCC_VALIDATOR_AUTOFIX_WARNINGS))
			}

			// Now we are sure that constant buffer is valid and ready to create descriptor.
			RefPtr<HRIShaderParamLocationDesc> ShaderParamConstantBuffersLocationDesc(new HRIShaderParamLocationDesc(ShaderInstanceDesc, GD_HRI_SHADER_PARAM_LOCATION_DESC_TYPE_CONSTANTBUFFER));
			for (auto const Definition : ConstantBuffer->InnerDefinitions)
			{
				HLSLVariable const* const ConstantBufferParam = HLSLDynamicCast<HLSLVariable const*>(Definition);
				if (ConstantBufferParam == nullptr)
					continue;

				HRIShaderParamDescType ParamType = GD_HRI_SHADER_PARAM_DESC_TYPE_UNKNOWN;
				Format                 ParamFormat = GD_FORMAT_UNKNOWN;
				/**/ if ((ConstantBufferParam->Type->Class == GD_HLSL_TYPE_CLASS_SCALAR) || (ConstantBufferParam->Type->Class == GD_HLSL_TYPE_CLASS_VECTOR))
				{
					HLSLVectorType const* const VectorType = HLSLDynamicCast<HLSLVectorType const*>(ConstantBufferParam->Type);
					HLSLTypeDataType      const DataType   = ((VectorType != nullptr) ? VectorType->DataType : static_cast<HLSLScalarType const*>(ConstantBufferParam->Type)->DataType);

					size_t     ParamFormatSize = GD_FORMAT_SIZE_UNKNOWN;
					FormatType ParamFormatType = GD_FORMAT_TYPE_UNKNOWN;
					switch (DataType)
					{
					case GD_HLSL_TYPE_DATA_TYPE_bool:
						ParamFormatSize = GD_FORMAT_SIZE_8BITS;
						ParamFormatType = GD_FORMAT_TYPE_UINT;
						break;

					case GD_HLSL_TYPE_DATA_TYPE_int:
						ParamFormatSize = GD_FORMAT_SIZE_32BITS;
						ParamFormatType = GD_FORMAT_TYPE_SINT;
						break;

					case GD_HLSL_TYPE_DATA_TYPE_float:
						ParamFormatSize = GD_FORMAT_SIZE_32BITS;
						ParamFormatType = GD_FORMAT_TYPE_FLOAT;
						break;

					case GD_HLSL_TYPE_DATA_TYPE_double:
						ParamFormatSize = GD_FORMAT_SIZE_64BITS;
						ParamFormatType = GD_FORMAT_TYPE_FLOAT;
						break;

					case GD_HLSL_TYPE_DATA_TYPE_dword:
					case GD_HLSL_TYPE_DATA_TYPE_uint:
						ParamFormatSize = GD_FORMAT_SIZE_32BITS;
						ParamFormatType = GD_FORMAT_TYPE_UINT;
						break;

					default:
						GD_NOT_IMPLEMENTED();
						break;
					}

					ParamType = GD_HRI_SHADER_PARAM_DESC_TYPE_FORMATABLE;
					ParamFormat = GD_FORMAT_MAKE(ParamFormatType, ParamFormatSize, ((VectorType != nullptr) ? VectorType->ComponentsNumber : 1));
				}
				else if (ConstantBufferParam->Type->Class == GD_HLSL_TYPE_CLASS_MATRIX)
				{
					HLSLMatrixType const* const MatrixType = static_cast<HLSLMatrixType const*>(ConstantBufferParam->Type);
					/**/ if ((MatrixType->RowsNumber == 4) && (MatrixType->ColumnsNumber == 4))
						ParamType = GD_HRI_SHADER_PARAM_DESC_TYPE_MATRIX4X4;
					else if ((MatrixType->RowsNumber == 3) && (MatrixType->ColumnsNumber == 3))
						ParamType = GD_HRI_SHADER_PARAM_DESC_TYPE_MATRIX3X3;
					else GD_NOT_IMPLEMENTED();
				}

				RefPtr<HRIShaderParamDesc>(new HRIShaderParamDesc(ShaderParamConstantBuffersLocationDesc.GetPointer(), ConstantBufferParam->Name, ParamType, ParamFormat, ConstantBufferParam->ArrayIndex));
			}
		}

		return true;
	}

	bool HLSLValidator::ValidateEntryPoint(HLSLScope const* const ParsedData, String const& EntryPointName, RefPtr<HRIShaderInstanceDesc>& ShaderInstanceDesc)
	{
		HLSLFunction const* const EntryPoint = ParsedData->FindFunction(EntryPointName);
		if (EntryPoint == nullptr)
		{	// No entry point was found.
			HLSLValidatorErrorDesc static const NoEntryFunctionError("entry point function named '%s' was not found.");
			self->RaiseError(NoEntryFunctionError, &EntryPointName[0]);
			self->RaiseExceptionWithCode(GD_HRI_SHADERCC_EXCEPTION_SYNTAX);

			return false;
		}

		UInt64 ShaderInputSemantics = 0;
		UInt64 ShaderOutputSemantics = 0;

		// Utilizing entry point return value.
		if (EntryPoint->Type->Class != GD_HLSL_TYPE_CLASS_VOID)
		{
#if (defined(GD_SHADERCC_VALIDATOR_VALIDATE_STYLE))
			HLSLValidatorWarningDec static const EntryReturnsNonVoidStyleWarning("style warning: entry point function '%s' should return void.");
			self->RaiseWarning(EntryReturnsNonVoidStyleWarning, &EntryPointName[0]);
#endif	// if (defined(GD_SHADERCC_VALIDATOR_VALIDATE_STYLE))

#if ((defined(GD_SHADERCC_VALIDATOR_AUTOFIX_WARNINGS)) && (defined(GD_SHADERCC_VALIDATOR_VALIDATE_STYLE)))
#	error 'Both 'GD_SHADERCC_VALIDATOR_AUTOFIX_WARNINGS' and 'GD_SHADERCC_VALIDATOR_VALIDATE_STYLE' but fix for 'EntryReturnsNonVoidStyleWarning' is not implemented.' 
#else	// if ((defined(GD_SHADERCC_VALIDATOR_AUTOFIX_WARNINGS)) && (defined(GD_SHADERCC_VALIDATOR_VALIDATE_STYLE)))

			/**/ if (EntryPoint->Type->Class == GD_HLSL_TYPE_CLASS_STRUCT)
			{	// Return value of entry point is structure that may contain some semantics.
				HLSLStruct const* const EntryPointReturnStruct = static_cast<HLSLStruct const*>(EntryPoint->Type);
				for (auto const Definition : EntryPointReturnStruct->InnerDefinitions)
				{
					HLSLVariable const* const EntryPointReturnStructField = static_cast<HLSLVariable const*>(Definition);
					if (EntryPointReturnStructField->ExprColon == nullptr)
					{
						HLSLValidatorErrorDesc static const EntryReturnValueNotUtilizedError("entry point function '%s' returns structure that contains unutilized field '%s'.");
						self->RaiseError(EntryReturnValueNotUtilizedError, &EntryPointName[0], &EntryPointReturnStructField[0]);
						self->RaiseExceptionWithCode(GD_HRI_SHADERCC_EXCEPTION_SYNTAX);

						return false;
					}

					if (!self->ValidateEntryPointArgumentExprColon(EntryPointReturnStructField->ExprColon, ShaderInputSemantics))
						return false;
				}
			}
			else if ((EntryPoint->Type->Class == GD_HLSL_TYPE_CLASS_SCALAR) || (EntryPoint->Type->Class == GD_HLSL_TYPE_CLASS_VECTOR))
			{	// Entry point returns scalar or vector value.
				HLSLSemantic const* const EntryPointSemantic = EntryPoint->Semantic;
				if (EntryPointSemantic == nullptr)
				{	// Entry point returns value that is not utilized with any semantic
					HLSLValidatorErrorDesc static const EntryReturnValueNotUtilizedError("entry point function '%s' returns value that is not utilized with any semantic.");
					self->RaiseError(EntryReturnValueNotUtilizedError, &EntryPointName[0]);
					self->RaiseExceptionWithCode(GD_HRI_SHADERCC_EXCEPTION_SYNTAX);

					return false;
				}

				if (!self->ValidateEntryPointArgumentExprColon(EntryPointSemantic, ShaderOutputSemantics)) 
					return false;
			}
			else
			{	// Entry point return something really strange.
				HLSLValidatorErrorDesc static const EntryReturnNotUtilizableValueError("entry point function '%s' returns value that cannot be utilized.");
				self->RaiseError(EntryReturnNotUtilizableValueError, &EntryPointName[0]);
				self->RaiseExceptionWithCode(GD_HRI_SHADERCC_EXCEPTION_SYNTAX);

				return false;
			}
#endif	// if ((defined(GD_SHADERCC_VALIDATOR_AUTOFIX_WARNINGS)) && (defined(GD_SHADERCC_VALIDATOR_VALIDATE_STYLE)))
		}

		// Utilizing entry point arguments.
		for (auto const EntryPointArgument : EntryPoint->Arguments)
		{
			UInt64* CurrentShaderSemantics = nullptr;
			/**/ if (EntryPointArgument->AccsessType == GD_HLSL_ARGUMENT_IN ) CurrentShaderSemantics = &ShaderInputSemantics;
			else if (EntryPointArgument->AccsessType == GD_HLSL_ARGUMENT_OUT) CurrentShaderSemantics = &ShaderOutputSemantics;
			else
			{
				HLSLValidatorErrorDesc static const InvalidAccessTypeError("invalid access type of argument '%s'");
				self->RaiseError(InvalidAccessTypeError, &EntryPointArgument->Name[0]);
				self->RaiseExceptionWithCode(GD_HRI_SHADERCC_EXCEPTION_SYNTAX);

				return false;
			}

			HLSLStruct const* const EntryPointArgumentStruct = HLSLDynamicCast<HLSLStruct const*>(EntryPointArgument->Type);
			if (EntryPointArgumentStruct != nullptr)
				for (auto const Definition : EntryPointArgumentStruct->InnerDefinitions)
				{
					HLSLVariable const* const EntryPointArgumentStructField = static_cast<HLSLVariable const*>(Definition);
					if (!self->ValidateEntryPointArgumentExprColon(EntryPointArgumentStructField->ExprColon, *CurrentShaderSemantics))
						return false;
				}
			else
				if (!self->ValidateEntryPointArgumentExprColon(EntryPointArgument->ExprColon, *CurrentShaderSemantics))
					return false;
		}

		ShaderInstanceDesc = new HRIShaderInstanceDesc(ShaderInputSemantics, ShaderOutputSemantics);
		return true;
	}

	bool HLSLValidator::ValidateEntryPointArgumentExprColon(HLSLExprColon const* const ExprColon, UInt64& ShaderSemanticsList)
	{
		HLSLSemantic const* const ArgumentSemanticHLSL = static_cast<HLSLSemantic const*>(ExprColon);
		HRISemantic const ArgumentSemanticHRI = HLSLSemanticToHRI(ArgumentSemanticHLSL->Semantic);
		if (ArgumentSemanticHRI == GD_HRI_SEMANTIC_UNKNOWN)
		{	/// @todo Do something here.
	//		HLSLValidatorErrorDesc static const UntranslatableSemanticError("Untranslatable semantic '%s' in argument '%s'");
	//		self->RaiseError(UntranslatableSemanticError, HLSLSemanticToStr(ArgumentHLSLSemantic->Semantic), &Argument->Name[0]);
	//		self->RaiseExceptionWithCode(GD_HRI_SHADERCC_EXCEPTION_SYNTAX);
	//	
	//		return false;
			return true;
		}

	//	if ((ShaderSemanticsList & GD_BIT(UInt64(ArgumentSemanticHRI + 1))) != 0)
	//	{
	//		HLSLValidatorErrorDesc static const InvalidAccessTypeError("entry point function already contains (in/out)put for '%s' semantic");
	//		self->RaiseError(InvalidAccessTypeError, HLSLSemanticToStr(ArgumentSemanticHLSL->Semantic));
	//		self->RaiseExceptionWithCode(GD_HRI_SHADERCC_EXCEPTION_SYNTAX);
	//
	//		return false;
	//	}

		ShaderSemanticsList |= GD_BIT(UInt64(ArgumentSemanticHRI + 1));
		return true;
	}

	HRIShaderInstanceDesc* HLSLValidator::ValidateAndGenerateDescription(HLSLScope const* const ParsedData, String const& EntryPointName)
	{
		RefPtr<HRIShaderInstanceDesc> ShaderInstanceDesc(nullptr);
		if (!self->ValidateEntryPoint(ParsedData, EntryPointName, ShaderInstanceDesc))
			return nullptr;

		if (!self->ValidateConstantBuffersParameters(ParsedData, ShaderInstanceDesc.GetPointer()))
			return nullptr;

		if (!self->ValidateResourceParameters(ParsedData, ShaderInstanceDesc.GetPointer()))
			return nullptr;

		return ShaderInstanceDesc.Release();
	}

GD_NAMESPACE_END
