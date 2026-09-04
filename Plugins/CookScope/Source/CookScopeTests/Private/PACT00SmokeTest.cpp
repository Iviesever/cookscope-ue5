#include "CookScopeAuditCommandlet.h"
#include "CookScopeEditorModule.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "ImageUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Widgets/Docking/SDockTab.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCookScopePACT00SmokeTest,
	"CookScope.PACT00.EditorAndCommandletContracts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCookScopePACT00SmokeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	TestEqual(TEXT("The public tab identifier is stable"), FCookScopeEditorModule::GetTabName(), FName(TEXT("CookScope")));
	TestNotNull(TEXT("The audit commandlet class is registered"), UCookScopeAuditCommandlet::StaticClass());
	const TSharedPtr<SDockTab> CookScopeTab = FCookScopeEditorModule::InvokeTab();
	TestTrue(TEXT("The registered CookScope tab can be invoked"), CookScopeTab.IsValid());
	if (CookScopeTab.IsValid())
	{
		TestEqual(TEXT("The spawned tab has nomad role"), CookScopeTab->GetTabRole(), ETabRole::NomadTab);
		TestEqual(TEXT("The tab hosts the production CookScope panel"), CookScopeTab->GetContent()->GetTypeAsString(), FString(TEXT("SCookScopePanel")));

		FString ScreenshotPath;
		if (FParse::Value(FCommandLine::Get(), TEXT("CookScopeScreenshotPath="), ScreenshotPath))
		{
			ScreenshotPath = FPaths::ConvertRelativePathToFull(ScreenshotPath);
			TArray<FColor> Pixels;
			FIntVector Size = FIntVector::ZeroValue;
			const bool Captured = FSlateApplication::Get().TakeScreenshot(CookScopeTab->GetContent(), Pixels, Size);
			TestTrue(TEXT("The production panel can be rendered for documentation"), Captured);
			TestTrue(TEXT("The rendered panel has a useful width"), Size.X >= 960);
			TestTrue(TEXT("The rendered panel has a useful height"), Size.Y >= 560);
			if (Captured && Size.X > 0 && Size.Y > 0)
			{
				for (FColor& Pixel : Pixels)
				{
					Pixel.A = 255;
				}
				TArray64<uint8> CompressedPng;
				FImageUtils::PNGCompressImageArray(
					Size.X,
					Size.Y,
					TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()),
					CompressedPng);
				IFileManager::Get().MakeDirectory(*FPaths::GetPath(ScreenshotPath), true);
				TestTrue(TEXT("The rendered panel PNG can be written"), FFileHelper::SaveArrayToFile(CompressedPng, *ScreenshotPath));
			}
		}
		CookScopeTab->RequestCloseTab();
	}
	return true;
}

#endif
