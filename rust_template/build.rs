fn main() {
    println!("cargo:rerun-if-changed=bindings/eqnx.h");

    let bindings = bindgen::Builder::default()
        .header("bindings/eqnx.h")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks::new()))
        .generate()
        .expect("Unable to generate bindings");

    bindings
        .write_to_file("bindings/eqnx.rs")
        .expect("Couldn't write bindings!");
}
