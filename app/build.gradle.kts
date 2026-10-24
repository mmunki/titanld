import org.jetbrains.kotlin.gradle.dsl.JvmTarget

plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.plugin.compose")
}

val titanVersionName = "3.0.0"
val titanVersionCode = 4

android {
    namespace = "io.github.mmunki.titanld"
    compileSdk {
        version = release(37) {
            minorApiLevel = 0
        }
    }

    defaultConfig {
        applicationId = "io.github.mmunki.titanld"
        minSdk = 26
        targetSdk = 37
        versionCode = titanVersionCode
        versionName = titanVersionName
        ndk {
            abiFilters += listOf("arm64-v8a", "armeabi-v7a", "x86", "x86_64")
        }
    }

    sourceSets["main"].apply {
        manifest.srcFile("src/main/AndroidManifest.xml")
        res.srcDir(rootProject.file("res"))
        assets.srcDir(layout.buildDirectory.dir("generated/assets").get().asFile)
        jniLibs.srcDir(rootProject.layout.projectDirectory.dir("build/native"))
    }

    buildFeatures {
        compose = true
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    packaging {
        jniLibs {
            useLegacyPackaging = true
        }
    }

    dependenciesInfo {
        includeInApk = false
        includeInBundle = false
    }

    val keystorePath = System.getenv("TITANLD_KEYSTORE")
    signingConfigs {
        if (keystorePath != null) {
            create("release") {
                storeFile = file(keystorePath)
                storePassword = System.getenv("TITANLD_KEYSTORE_PASSWORD") ?: "titanld-build"
                keyAlias = System.getenv("TITANLD_KEY_ALIAS") ?: "titanld"
                keyPassword = System.getenv("TITANLD_KEY_PASSWORD") ?: "titanld-build"
                enableV3Signing = true
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            signingConfig = signingConfigs.findByName("release")
                ?: signingConfigs.getByName("debug")
        }
    }
}

kotlin {
    compilerOptions {
        jvmTarget = JvmTarget.JVM_17
    }
}

dependencies {
    implementation("androidx.activity:activity-compose:1.13.0")
    implementation("top.yukonga.miuix.kmp:miuix-ui:0.9.4")
    implementation("top.yukonga.miuix.kmp:miuix-icons:0.9.4")
}

val prepareAssets by tasks.registering(Copy::class) {
    from(rootProject.file("assets")) {
        exclude("licenses/MobileGlues-LGPL-2.1.txt")
    }
    from(rootProject.file("LICENSE")) {
        rename { "LICENSE.txt" }
    }
    into(layout.buildDirectory.dir("generated/assets"))
}

val buildNative by tasks.registering(Exec::class) {
    workingDir(rootProject.projectDir)
    commandLine("bash", "build.sh", "--native-only")
    inputs.files(fileTree(rootProject.file("src/native")) { include("**/*.c", "**/*.h") })
    inputs.file(rootProject.file("tests/glsl_test.c"))
    inputs.file(rootProject.file("build.sh"))
    outputs.dir(rootProject.layout.projectDirectory.dir("build/native"))
}

tasks.named("preBuild").configure {
    dependsOn(prepareAssets, buildNative)
}
